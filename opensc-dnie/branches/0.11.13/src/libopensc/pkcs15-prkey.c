/*
 * pkcs15-prkey.c: PKCS #15 private key functions
 *
 * Copyright (C) 2001, 2002  Juha Yrjölä <juha.yrjola@iki.fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "internal.h"
#include "pkcs15.h"
#include "asn1.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

static const struct sc_asn1_entry c_asn1_com_key_attr[] = {
	{ "iD",		 SC_ASN1_PKCS15_ID, SC_ASN1_TAG_OCTET_STRING, 0, NULL, NULL },
	{ "usage",	 SC_ASN1_BIT_FIELD, SC_ASN1_TAG_BIT_STRING, 0, NULL, NULL },
	{ "native",	 SC_ASN1_BOOLEAN, SC_ASN1_TAG_BOOLEAN, SC_ASN1_OPTIONAL, NULL, NULL },
	{ "accessFlags", SC_ASN1_BIT_FIELD, SC_ASN1_TAG_BIT_STRING, SC_ASN1_OPTIONAL, NULL, NULL },
	{ "keyReference",SC_ASN1_INTEGER, SC_ASN1_TAG_INTEGER, SC_ASN1_OPTIONAL, NULL, NULL },
	{ NULL, 0, 0, 0, NULL, NULL }
};

static const struct sc_asn1_entry c_asn1_com_prkey_attr[] = {
        /* FIXME */
	{ NULL, 0, 0, 0, NULL, NULL }
};

static const struct sc_asn1_entry c_asn1_rsakey_attr[] = {
	{ "value",	   SC_ASN1_PATH, SC_ASN1_TAG_SEQUENCE | SC_ASN1_CONS, 0, NULL, NULL },
	{ "modulusLength", SC_ASN1_INTEGER, SC_ASN1_TAG_INTEGER, 0, NULL, NULL },
	{ "keyInfo",	   SC_ASN1_INTEGER, SC_ASN1_TAG_INTEGER, SC_ASN1_OPTIONAL, NULL, NULL },
	{ NULL, 0, 0, 0, NULL, NULL }
};

static const struct sc_asn1_entry c_asn1_prk_rsa_attr[] = {
	{ "privateRSAKeyAttributes", SC_ASN1_STRUCT, SC_ASN1_TAG_SEQUENCE | SC_ASN1_CONS, 0, NULL, NULL },
	{ NULL, 0, 0, 0, NULL, NULL }
};

static const struct sc_asn1_entry c_asn1_gostr3410key_attr[] = {
	{ "value",         SC_ASN1_PATH, SC_ASN1_TAG_SEQUENCE | SC_ASN1_CONS, 0, NULL, NULL },
	{ "params_r3410",  SC_ASN1_INTEGER, SC_ASN1_TAG_INTEGER, 0, NULL, NULL },
	{ "params_r3411",  SC_ASN1_INTEGER, SC_ASN1_TAG_INTEGER, SC_ASN1_OPTIONAL, NULL, NULL },
	{ "params_28147",  SC_ASN1_INTEGER, SC_ASN1_TAG_INTEGER, SC_ASN1_OPTIONAL, NULL, NULL },
	{ NULL, 0, 0, 0, NULL, NULL }
};

static const struct sc_asn1_entry c_asn1_prk_gostr3410_attr[] = {
	{ "privateGOSTR3410KeyAttributes", SC_ASN1_STRUCT, SC_ASN1_TAG_SEQUENCE | SC_ASN1_CONS, 0, NULL, NULL },
	{ NULL, 0, 0, 0, NULL, NULL }
};

static const struct sc_asn1_entry c_asn1_dsakey_i_p_attr[] = {
	{ "path",	SC_ASN1_PATH, SC_ASN1_TAG_SEQUENCE | SC_ASN1_CONS, 0, NULL, NULL },
	{ NULL, 0, 0, 0, NULL, NULL }
};

static const struct sc_asn1_entry c_asn1_dsakey_value_attr[] = {
	{ "path",	SC_ASN1_PATH, SC_ASN1_TAG_SEQUENCE | SC_ASN1_CONS, 0, NULL, NULL },
	{ "pathProtected",SC_ASN1_STRUCT, SC_ASN1_CTX | 1 | SC_ASN1_CONS, 0, NULL, NULL},
	{ NULL, 0, 0, 0, NULL, NULL }
};

static const struct sc_asn1_entry c_asn1_dsakey_attr[] = {
	{ "value",	SC_ASN1_CHOICE, 0, 0, NULL, NULL },
	{ NULL, 0, 0, 0, NULL, NULL }
};

static const struct sc_asn1_entry c_asn1_prk_dsa_attr[] = {
	{ "privateDSAKeyAttributes", SC_ASN1_STRUCT, SC_ASN1_TAG_SEQUENCE | SC_ASN1_CONS, 0, NULL, NULL },
	{ NULL, 0, 0, 0, NULL, NULL }
};

static const struct sc_asn1_entry c_asn1_prkey[] = {
	{ "privateRSAKey", SC_ASN1_PKCS15_OBJECT, SC_ASN1_TAG_SEQUENCE | SC_ASN1_CONS, SC_ASN1_OPTIONAL, NULL, NULL },
	{ "privateDSAKey", SC_ASN1_PKCS15_OBJECT,  2 | SC_ASN1_CTX | SC_ASN1_CONS, SC_ASN1_OPTIONAL, NULL, NULL },
	{ "privateGOSTR3410Key", SC_ASN1_PKCS15_OBJECT, 3 | SC_ASN1_CTX | SC_ASN1_CONS, SC_ASN1_OPTIONAL, NULL, NULL },
	{ NULL, 0, 0, 0, NULL, NULL }
};

int sc_pkcs15_decode_prkdf_entry(struct sc_pkcs15_card *p15card,
				 struct sc_pkcs15_object *obj,
				 const u8 ** buf, size_t *buflen)
{
        sc_context_t *ctx = p15card->card->ctx;
        struct sc_pkcs15_prkey_info info;
	int r, gostr3410_params[3];
	struct sc_pkcs15_keyinfo_gostparams *keyinfo_gostparams;
	size_t usage_len = sizeof(info.usage);
	size_t af_len = sizeof(info.access_flags);
	struct sc_asn1_entry asn1_com_key_attr[6], asn1_com_prkey_attr[1];
	struct sc_asn1_entry asn1_rsakey_attr[4], asn1_prk_rsa_attr[2];
	struct sc_asn1_entry asn1_dsakey_attr[2], asn1_prk_dsa_attr[2],
			asn1_dsakey_i_p_attr[2],
			asn1_dsakey_value_attr[3];
	struct sc_asn1_entry asn1_gostr3410key_attr[5], asn1_prk_gostr3410_attr[2];
	struct sc_asn1_entry asn1_prkey[4];
	struct sc_asn1_pkcs15_object rsa_prkey_obj = { obj, asn1_com_key_attr,
						       asn1_com_prkey_attr, asn1_prk_rsa_attr };
	struct sc_asn1_pkcs15_object dsa_prkey_obj = { obj, asn1_com_key_attr,
						       asn1_com_prkey_attr, asn1_prk_dsa_attr };
	struct sc_asn1_pkcs15_object gostr3410_prkey_obj =  { obj, asn1_com_key_attr,
						       asn1_com_prkey_attr,
						       asn1_prk_gostr3410_attr };

	sc_copy_asn1_entry(c_asn1_prkey, asn1_prkey);

	sc_copy_asn1_entry(c_asn1_prk_rsa_attr, asn1_prk_rsa_attr);
	sc_copy_asn1_entry(c_asn1_rsakey_attr, asn1_rsakey_attr);
	sc_copy_asn1_entry(c_asn1_prk_dsa_attr, asn1_prk_dsa_attr);
	sc_copy_asn1_entry(c_asn1_dsakey_attr, asn1_dsakey_attr);
	sc_copy_asn1_entry(c_asn1_dsakey_value_attr, asn1_dsakey_value_attr);
	sc_copy_asn1_entry(c_asn1_dsakey_i_p_attr, asn1_dsakey_i_p_attr);
	sc_copy_asn1_entry(c_asn1_prk_gostr3410_attr, asn1_prk_gostr3410_attr);
	sc_copy_asn1_entry(c_asn1_gostr3410key_attr, asn1_gostr3410key_attr);

	sc_copy_asn1_entry(c_asn1_com_prkey_attr, asn1_com_prkey_attr);
	sc_copy_asn1_entry(c_asn1_com_key_attr, asn1_com_key_attr);

	sc_format_asn1_entry(asn1_prkey + 0, &rsa_prkey_obj, NULL, 0);
	sc_format_asn1_entry(asn1_prkey + 1, &dsa_prkey_obj, NULL, 0);
	sc_format_asn1_entry(asn1_prkey + 2, &gostr3410_prkey_obj, NULL, 0);

	sc_format_asn1_entry(asn1_prk_rsa_attr + 0, asn1_rsakey_attr, NULL, 0);
	sc_format_asn1_entry(asn1_prk_dsa_attr + 0, asn1_dsakey_attr, NULL, 0);
	sc_format_asn1_entry(asn1_prk_gostr3410_attr + 0, asn1_gostr3410key_attr, NULL, 0);

	sc_format_asn1_entry(asn1_rsakey_attr + 0, &info.path, NULL, 0);
	sc_format_asn1_entry(asn1_rsakey_attr + 1, &info.modulus_length, NULL, 0);

	sc_format_asn1_entry(asn1_dsakey_attr + 0, asn1_dsakey_value_attr, NULL, 0);
	sc_format_asn1_entry(asn1_dsakey_value_attr + 0, &info.path, NULL, 0);
	sc_format_asn1_entry(asn1_dsakey_value_attr + 1, asn1_dsakey_i_p_attr, NULL, 0);
	sc_format_asn1_entry(asn1_dsakey_i_p_attr + 0, &info.path, NULL, 0);

	sc_format_asn1_entry(asn1_gostr3410key_attr + 0, &info.path, NULL, 0);
	sc_format_asn1_entry(asn1_gostr3410key_attr + 1, &gostr3410_params[0], NULL, 0);
	sc_format_asn1_entry(asn1_gostr3410key_attr + 2, &gostr3410_params[1], NULL, 0);
	sc_format_asn1_entry(asn1_gostr3410key_attr + 3, &gostr3410_params[2], NULL, 0);

	sc_format_asn1_entry(asn1_com_key_attr + 0, &info.id, NULL, 0);
	sc_format_asn1_entry(asn1_com_key_attr + 1, &info.usage, &usage_len, 0);
	sc_format_asn1_entry(asn1_com_key_attr + 2, &info.native, NULL, 0);
	sc_format_asn1_entry(asn1_com_key_attr + 3, &info.access_flags, &af_len, 0);
	sc_format_asn1_entry(asn1_com_key_attr + 4, &info.key_reference, NULL, 0);

        /* Fill in defaults */
        memset(&info, 0, sizeof(info));
	info.key_reference = -1;
	info.native = 1;
	memset(gostr3410_params, 0, sizeof(gostr3410_params));

	r = sc_asn1_decode_choice(ctx, asn1_prkey, *buf, *buflen, buf, buflen);
	if (r == SC_ERROR_ASN1_END_OF_CONTENTS)
		return r;
	SC_TEST_RET(ctx, r, "ASN.1 decoding failed");
	if (asn1_prkey[0].flags & SC_ASN1_PRESENT) {
		obj->type = SC_PKCS15_TYPE_PRKEY_RSA;
	} else if (asn1_prkey[1].flags & SC_ASN1_PRESENT) {
		obj->type = SC_PKCS15_TYPE_PRKEY_DSA;
		/* If the value was indirect-protected, mark the path */
		if (asn1_dsakey_i_p_attr[0].flags & SC_ASN1_PRESENT)
			info.path.type = SC_PATH_TYPE_PATH_PROT;
	} else if (asn1_prkey[2].flags & SC_ASN1_PRESENT) {
		obj->type = SC_PKCS15_TYPE_PRKEY_GOSTR3410;
		assert(info.modulus_length == 0);
		info.modulus_length = SC_PKCS15_GOSTR3410_KEYSIZE;
		assert(info.params_len == 0);
		info.params_len = sizeof(struct sc_pkcs15_keyinfo_gostparams);
		info.params = malloc(info.params_len);
		if (info.params == NULL)
			SC_FUNC_RETURN(ctx, 0, SC_ERROR_OUT_OF_MEMORY);
		assert(sizeof(*keyinfo_gostparams) == info.params_len);
		keyinfo_gostparams = info.params;
		keyinfo_gostparams->gostr3410 = gostr3410_params[0];
		keyinfo_gostparams->gostr3411 = gostr3410_params[1];
		keyinfo_gostparams->gost28147 = gostr3410_params[2];
	} else {
		sc_error(ctx, "Neither RSA or DSA or GOSTR3410 key in PrKDF entry.\n");
		SC_FUNC_RETURN(ctx, 0, SC_ERROR_INVALID_ASN1_OBJECT);
	}
	r = sc_pkcs15_make_absolute_path(&p15card->file_app->path, &info.path);
	if (r < 0) {
		if (info.params)
			free(info.params);
		return r;
	}

        /* OpenSC 0.11.4 and older encoded "keyReference" as a negative
           value. Fixed in 0.11.5 we need to add a hack, so old cards
           continue to work. */
        if (p15card->flags & SC_PKCS15_CARD_FLAG_FIX_INTEGERS) {
                if (info.key_reference < -1) {
                        info.key_reference += 256;
                }
        }

	obj->data = malloc(sizeof(info));
	if (obj->data == NULL) {
		if (info.params)
			free(info.params);
		SC_FUNC_RETURN(ctx, 0, SC_ERROR_OUT_OF_MEMORY);
	}
	memcpy(obj->data, &info, sizeof(info));

	return 0;
}

int sc_pkcs15_encode_prkdf_entry(sc_context_t *ctx,
				 const struct sc_pkcs15_object *obj,
				 u8 **buf, size_t *buflen)
{
	struct sc_asn1_entry asn1_com_key_attr[6], asn1_com_prkey_attr[1];
	struct sc_asn1_entry asn1_rsakey_attr[4], asn1_prk_rsa_attr[2];
	struct sc_asn1_entry asn1_dsakey_attr[2], asn1_prk_dsa_attr[2],
			asn1_dsakey_value_attr[3],
			asn1_dsakey_i_p_attr[2];
	struct sc_asn1_entry asn1_gostr3410key_attr[5], asn1_prk_gostr3410_attr[2];
	struct sc_asn1_entry asn1_prkey[4];
	struct sc_asn1_pkcs15_object rsa_prkey_obj = { (struct sc_pkcs15_object *) obj, asn1_com_key_attr,
						       asn1_com_prkey_attr, asn1_prk_rsa_attr };
	struct sc_asn1_pkcs15_object dsa_prkey_obj = { (struct sc_pkcs15_object *) obj, asn1_com_key_attr,
						       asn1_com_prkey_attr, asn1_prk_dsa_attr };
	struct sc_asn1_pkcs15_object gostr3410_prkey_obj =  { (struct sc_pkcs15_object *) obj,
						       asn1_com_key_attr, asn1_com_prkey_attr,
						       asn1_prk_gostr3410_attr };
	struct sc_pkcs15_prkey_info *prkey =
                (struct sc_pkcs15_prkey_info *) obj->data;
	struct sc_pkcs15_keyinfo_gostparams *keyinfo_gostparams;
	int r;
	size_t af_len, usage_len;

	sc_copy_asn1_entry(c_asn1_prkey, asn1_prkey);

	sc_copy_asn1_entry(c_asn1_prk_rsa_attr, asn1_prk_rsa_attr);
	sc_copy_asn1_entry(c_asn1_rsakey_attr, asn1_rsakey_attr);
	sc_copy_asn1_entry(c_asn1_prk_dsa_attr, asn1_prk_dsa_attr);
	sc_copy_asn1_entry(c_asn1_dsakey_attr, asn1_dsakey_attr);
	sc_copy_asn1_entry(c_asn1_dsakey_value_attr, asn1_dsakey_value_attr);
	sc_copy_asn1_entry(c_asn1_dsakey_i_p_attr, asn1_dsakey_i_p_attr);
	sc_copy_asn1_entry(c_asn1_prk_gostr3410_attr, asn1_prk_gostr3410_attr);
	sc_copy_asn1_entry(c_asn1_gostr3410key_attr, asn1_gostr3410key_attr);

	sc_copy_asn1_entry(c_asn1_com_prkey_attr, asn1_com_prkey_attr);
	sc_copy_asn1_entry(c_asn1_com_key_attr, asn1_com_key_attr);

	switch (obj->type) {
	case SC_PKCS15_TYPE_PRKEY_RSA:
		sc_format_asn1_entry(asn1_prkey + 0, &rsa_prkey_obj, NULL, 1);
		sc_format_asn1_entry(asn1_prk_rsa_attr + 0, asn1_rsakey_attr, NULL, 1);
		sc_format_asn1_entry(asn1_rsakey_attr + 0, &prkey->path, NULL, 1);
		sc_format_asn1_entry(asn1_rsakey_attr + 1, &prkey->modulus_length, NULL, 1);
		break;
	case SC_PKCS15_TYPE_PRKEY_DSA:
		sc_format_asn1_entry(asn1_prkey + 1, &dsa_prkey_obj, NULL, 1);
		sc_format_asn1_entry(asn1_prk_dsa_attr + 0, asn1_dsakey_value_attr, NULL, 1);
		if (prkey->path.type != SC_PATH_TYPE_PATH_PROT) {
			/* indirect: just add the path */
			sc_format_asn1_entry(asn1_dsakey_value_attr + 0,
					&prkey->path, NULL, 1);
		} else {
			/* indirect-protected */
			sc_format_asn1_entry(asn1_dsakey_value_attr + 1,
					asn1_dsakey_i_p_attr, NULL, 1);
			sc_format_asn1_entry(asn1_dsakey_i_p_attr + 0,
					&prkey->path, NULL, 1);
		}
                break;
	case SC_PKCS15_TYPE_PRKEY_GOSTR3410:
		sc_format_asn1_entry(asn1_prkey + 2, &gostr3410_prkey_obj, NULL, 1);
		sc_format_asn1_entry(asn1_prk_gostr3410_attr + 0, asn1_gostr3410key_attr, NULL, 1);
		sc_format_asn1_entry(asn1_gostr3410key_attr + 0, &prkey->path, NULL, 1);
		if (prkey->params_len == sizeof(*keyinfo_gostparams))
		{
			keyinfo_gostparams = prkey->params;
			sc_format_asn1_entry(asn1_gostr3410key_attr + 1,
					&keyinfo_gostparams->gostr3410, NULL, 1);
			sc_format_asn1_entry(asn1_gostr3410key_attr + 2,
					&keyinfo_gostparams->gostr3411, NULL, 1);
			sc_format_asn1_entry(asn1_gostr3410key_attr + 3,
					&keyinfo_gostparams->gost28147, NULL, 1);
		}
		break;
	default:
		sc_error(ctx, "Invalid private key type: %X\n", obj->type);
		SC_FUNC_RETURN(ctx, 0, SC_ERROR_INTERNAL);
		break;
	}
	sc_format_asn1_entry(asn1_com_key_attr + 0, &prkey->id, NULL, 1);
	usage_len = sizeof(prkey->usage);
	sc_format_asn1_entry(asn1_com_key_attr + 1, &prkey->usage, &usage_len, 1);
	if (prkey->native == 0)
		sc_format_asn1_entry(asn1_com_key_attr + 2, &prkey->native, NULL, 1);
	if (prkey->access_flags) {
		af_len = sizeof(prkey->access_flags);
		sc_format_asn1_entry(asn1_com_key_attr + 3, &prkey->access_flags, &af_len, 1);
	}
	if (prkey->key_reference >= 0)
		sc_format_asn1_entry(asn1_com_key_attr + 4, &prkey->key_reference, NULL, 1);
	r = sc_asn1_encode(ctx, asn1_prkey, buf, buflen);

	return r;
}

/*
 * Store private keys on the card, encrypted
 */
static const struct sc_asn1_entry	c_asn1_dsa_prkey_obj[] = {
	{ "privateKey", SC_ASN1_OCTET_STRING, SC_ASN1_TAG_INTEGER, SC_ASN1_ALLOC, NULL, NULL },
	{ NULL, 0, 0, 0, NULL, NULL }
};

static int
sc_pkcs15_encode_prkey_dsa(sc_context_t *ctx,
		struct sc_pkcs15_prkey_dsa *key,
		u8 **buf, size_t *buflen)
{
	struct sc_asn1_entry	asn1_dsa_prkey_obj[2];

	sc_copy_asn1_entry(c_asn1_dsa_prkey_obj, asn1_dsa_prkey_obj);
	sc_format_asn1_entry(asn1_dsa_prkey_obj + 0,
			key->priv.data, &key->priv.len, 1);

	return sc_asn1_encode(ctx, asn1_dsa_prkey_obj, buf, buflen);
}

static int
sc_pkcs15_decode_prkey_dsa(sc_context_t *ctx,
		struct sc_pkcs15_prkey_dsa *key,
		const u8 *buf, size_t buflen)
{
	struct sc_asn1_entry	asn1_dsa_prkey_obj[2];

	sc_copy_asn1_entry(c_asn1_dsa_prkey_obj, asn1_dsa_prkey_obj);
	sc_format_asn1_entry(asn1_dsa_prkey_obj + 0,
			&key->priv.data, &key->priv.len, 0);

	return sc_asn1_decode(ctx, asn1_dsa_prkey_obj, buf, buflen, NULL, NULL);
}

int
sc_pkcs15_encode_prkey(sc_context_t *ctx,
		struct sc_pkcs15_prkey *key,
		u8 **buf, size_t *len)
{
	if (key->algorithm == SC_ALGORITHM_DSA)
		return sc_pkcs15_encode_prkey_dsa(ctx, &key->u.dsa, buf, len);
	sc_error(ctx, "Cannot encode private key type %u.\n",
			key->algorithm);
	return SC_ERROR_NOT_SUPPORTED;
}

int
sc_pkcs15_decode_prkey(sc_context_t *ctx,
		struct sc_pkcs15_prkey *key,
		const u8 *buf, size_t len)
{
	if (key->algorithm == SC_ALGORITHM_DSA)
		return sc_pkcs15_decode_prkey_dsa(ctx, &key->u.dsa, buf, len);
	sc_error(ctx, "Cannot decode private key type %u.\n",
			key->algorithm);
	return SC_ERROR_NOT_SUPPORTED;
}

int
sc_pkcs15_read_prkey(struct sc_pkcs15_card *p15card,
		const struct sc_pkcs15_object *obj,
		const char *passphrase,
		struct sc_pkcs15_prkey **out)
{
	sc_context_t *ctx = p15card->card->ctx;
	struct sc_pkcs15_prkey_info *info;
	struct sc_pkcs15_prkey key;
	sc_path_t path;
	u8 *data = NULL;
	size_t len;
	int r;

	memset(&key, 0, sizeof(key));
	switch (obj->type) {
	case SC_PKCS15_TYPE_PRKEY_RSA:
		key.algorithm = SC_ALGORITHM_RSA;
		break;
	case SC_PKCS15_TYPE_PRKEY_DSA:
		key.algorithm = SC_ALGORITHM_DSA;
		break;
	default:
		sc_error(ctx, "Unsupported object type.\n");
		return SC_ERROR_NOT_SUPPORTED;
	}
	info = (struct sc_pkcs15_prkey_info *) obj->data;
	if (info->native) {
		sc_error(ctx, "Private key is native, will not read.");
		return SC_ERROR_NOT_ALLOWED;
	}

	path = info->path;
	if (path.type == SC_PATH_TYPE_PATH_PROT)
		path.type = SC_PATH_TYPE_PATH;

	r = sc_pkcs15_read_file(p15card, &path, &data, &len, NULL);
	if (r < 0) {
		sc_error(ctx, "Unable to read private key file.\n");
		return r;
	}

	/* Is this a protected file? */
	if (info->path.type == SC_PATH_TYPE_PATH_PROT) {
		u8 *clear;
		size_t clear_len;

		if (passphrase == NULL) {
			r = SC_ERROR_PASSPHRASE_REQUIRED;
			goto fail;
		}
		r = sc_pkcs15_unwrap_data(ctx,
				passphrase,
				data, len,
				&clear, &clear_len);
		if (r < 0)  {
			sc_error(ctx, "Failed to unwrap privat key.");
			goto fail;
		}
		free(data);
		data = clear;
		len = clear_len;
	}

	r = sc_pkcs15_decode_prkey(ctx, &key, data, len);
	if (r < 0) {
		sc_error(ctx, "Unable to decode private key");
		goto fail;
	}

	*out = (struct sc_pkcs15_prkey *) malloc(sizeof(key));
	if (*out == NULL) {
		r = SC_ERROR_OUT_OF_MEMORY;
		goto fail;
	}

	**out = key;
	free(data);
	return 0;

fail:	if (data)
		free(data);
	return r;
}

void
sc_pkcs15_erase_prkey(struct sc_pkcs15_prkey *key)
{
	assert(key != NULL);
	switch (key->algorithm) {
	case SC_ALGORITHM_RSA:
		free(key->u.rsa.modulus.data);
		free(key->u.rsa.exponent.data);
		free(key->u.rsa.d.data);
		free(key->u.rsa.p.data);
		free(key->u.rsa.q.data);
		free(key->u.rsa.iqmp.data);
		free(key->u.rsa.dmp1.data);
		free(key->u.rsa.dmq1.data);
		break;
	case SC_ALGORITHM_DSA:
		free(key->u.dsa.pub.data);
		free(key->u.dsa.p.data);
		free(key->u.dsa.q.data);
		free(key->u.dsa.g.data);
		free(key->u.dsa.priv.data);
		break;
	case SC_ALGORITHM_GOSTR3410:
		assert(key->u.gostr3410.d.data);
		free(key->u.gostr3410.d.data);
		break;
	}
	sc_mem_clear(key, sizeof(key));
}

void
sc_pkcs15_free_prkey(struct sc_pkcs15_prkey *key)
{
	sc_pkcs15_erase_prkey(key);
	free(key);
}

void sc_pkcs15_free_prkey_info(sc_pkcs15_prkey_info_t *key)
{
	if (key->subject)
		free(key->subject);
	if (key->params)
		free(key->params);
	free(key);
}
