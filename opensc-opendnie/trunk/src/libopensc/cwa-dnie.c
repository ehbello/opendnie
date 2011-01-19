/*
 * cwa-dnie.c: DNIe data provider for CWA SM handling
 * 
 * Copyright (C) 2010 Juan Antonio Martinez <jonsito@terra.es>
 *
 * This work is derived from many sources at OpenSC Project site,
 * (see references) and the information made public by Spanish 
 * Direccion General de la Policia y de la Guardia Civil
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

#define __SM_DNIE_C__
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_OPENSSL   /* empty file without openssl */

#include <stdlib.h>
#include <string.h>

#include "opensc.h"
#include "cardctl.h"
#include "internal.h"
#include "compression.h"

#include "cwa14890.h"

#include <openssl/x509.h>
#include <openssl/evp.h>

/********************* Keys and certificates as published by DGP ********/

static u8 icc_root_ca_modulus[] = {
    0xEA, 0xDE, 0xDA, 0x45, 0x53, 0x32, 0x94, 0x50, 0x39, 0xDA, 0xA4, 0x04,
    0xC8, 0xEB, 0xC4, 0xD3, 0xB7, 0xF5, 0xDC, 0x86, 0x92, 0x83, 0xCD, 0xEA,
    0x2F, 0x10, 0x1E, 0x2A, 0xB5, 0x4F, 0xB0, 0xD0, 0xB0, 0x3D, 0x8F, 0x03,
    0x0D, 0xAF, 0x24, 0x58, 0x02, 0x82, 0x88, 0xF5, 0x4C, 0xE5, 0x52, 0xF8,
    0xFA, 0x57, 0xAB, 0x2F, 0xB1, 0x03, 0xB1, 0x12, 0x42, 0x7E, 0x11, 0x13,
    0x1D, 0x1D, 0x27, 0xE1, 0x0A, 0x5B, 0x50, 0x0E, 0xAA, 0xE5, 0xD9, 0x40,
    0x30, 0x1E, 0x30, 0xEB, 0x26, 0xC3, 0xE9, 0x06, 0x6B, 0x25, 0x71, 0x56,
    0xED, 0x63, 0x9D, 0x70, 0xCC, 0xC0, 0x90, 0xB8, 0x63, 0xAF, 0xBB, 0x3B,
    0xFE, 0xD8, 0xC1, 0x7B, 0xE7, 0x67, 0x30, 0x34, 0xB9, 0x82, 0x3E, 0x97,
    0x7E, 0xD6, 0x57, 0x25, 0x29, 0x27, 0xF9, 0x57, 0x5B, 0x9F, 0xFF, 0x66,
    0x91, 0xDB, 0x64, 0xF8, 0x0B, 0x5E, 0x92, 0xCD
};

static u8 icc_root_ca_public_exponent[] = {
   0x01, 0x00, 0x01
};

static u8 ifd_modulus [] = {
   0xdb, 0x2c, 0xb4, 0x1e, 0x11, 0x2b, 0xac, 0xfa, 0x2b, 0xd7, 0xc3, 0xd3,
   0xd7, 0x96, 0x7e, 0x84, 0xfb, 0x94, 0x34, 0xfc, 0x26, 0x1f, 0x9d, 0x09,
   0x0a, 0x89, 0x83, 0x94, 0x7d, 0xaf, 0x84, 0x88, 0xd3, 0xdf, 0x8f, 0xbd,
   0xcc, 0x1f, 0x92, 0x49, 0x35, 0x85, 0xe1, 0x34, 0xa1, 0xb4, 0x2d, 0xe5,
   0x19, 0xf4, 0x63, 0x24, 0x4d, 0x7e, 0xd3, 0x84, 0xe2, 0x6d, 0x51, 0x6c,
   0xc7, 0xa4, 0xff, 0x78, 0x95, 0xb1, 0x99, 0x21, 0x40, 0x04, 0x3a, 0xac,
   0xad, 0xfc, 0x12, 0xe8, 0x56, 0xb2, 0x02, 0x34, 0x6a, 0xf8, 0x22, 0x6b,
   0x1a, 0x88, 0x21, 0x37, 0xdc, 0x3c, 0x5a, 0x57, 0xf0, 0xd2, 0x81, 0x5c,
   0x1f, 0xcd, 0x4b, 0xb4, 0x6f, 0xa9, 0x15, 0x7f, 0xdf, 0xfd, 0x79, 0xec,
   0x3a, 0x10, 0xa8, 0x24, 0xcc, 0xc1, 0xeb, 0x3c, 0xe0, 0xb6, 0xb4, 0x39,
   0x6a, 0xe2, 0x36, 0x59, 0x00, 0x16, 0xba, 0x69
};

static u8 ifd_public_exponent [] = {
   0x01, 0x00, 0x01
};

static u8 ifd_private_exponent [] = {
   0x18, 0xb4, 0x4a, 0x3d, 0x15, 0x5c, 0x61, 0xeb, 0xf4, 0xe3, 0x26, 0x1c,
   0x8b, 0xb1, 0x57, 0xe3, 0x6f, 0x63, 0xfe, 0x30, 0xe9, 0xaf, 0x28, 0x89,
   0x2b, 0x59, 0xe2, 0xad, 0xeb, 0x18, 0xcc, 0x8c, 0x8b, 0xad, 0x28, 0x4b,
   0x91, 0x65, 0x81, 0x9c, 0xa4, 0xde, 0xc9, 0x4a, 0xa0, 0x6b, 0x69, 0xbc,
   0xe8, 0x17, 0x06, 0xd1, 0xc1, 0xb6, 0x68, 0xeb, 0x12, 0x86, 0x95, 0xe5,
   0xf7, 0xfe, 0xde, 0x18, 0xa9, 0x08, 0xa3, 0x01, 0x1a, 0x64, 0x6a, 0x48,
   0x1d, 0x3e, 0xa7, 0x1d, 0x8a, 0x38, 0x7d, 0x47, 0x46, 0x09, 0xbd, 0x57,
   0xa8, 0x82, 0xb1, 0x82, 0xe0, 0x47, 0xde, 0x80, 0xe0, 0x4b, 0x42, 0x21,
   0x41, 0x6b, 0xd3, 0x9d, 0xfa, 0x1f, 0xac, 0x03, 0x00, 0x64, 0x19, 0x62,
   0xad, 0xb1, 0x09, 0xe2, 0x8c, 0xaf, 0x50, 0x06, 0x1b, 0x68, 0xc9, 0xca,
   0xbd, 0x9b, 0x00, 0x31, 0x3c, 0x0f, 0x46, 0xed
};

/* Intermediate CA certificate in CVC format (Card verifiable certificate) */
static u8 C_CV_CA_CS_AUT_cert [] = {
  0x7f, 0x21, 0x81, 0xce, 0x5f, 0x37, 0x81, 0x80, 0x3c, 0xba, 0xdc, 0x36,
  0x84, 0xbe, 0xf3, 0x20, 0x41, 0xad, 0x15, 0x50, 0x89, 0x25, 0x8d, 0xfd,
  0x20, 0xc6, 0x91, 0x15, 0xd7, 0x2f, 0x9c, 0x38, 0xaa, 0x99, 0xad, 0x6c,
  0x1a, 0xed, 0xfa, 0xb2, 0xbf, 0xac, 0x90, 0x92, 0xfc, 0x70, 0xcc, 0xc0,
  0x0c, 0xaf, 0x48, 0x2a, 0x4b, 0xe3, 0x1a, 0xfd, 0xbd, 0x3c, 0xbc, 0x8c,
  0x83, 0x82, 0xcf, 0x06, 0xbc, 0x07, 0x19, 0xba, 0xab, 0xb5, 0x6b, 0x6e,
  0xc8, 0x07, 0x60, 0xa4, 0xa9, 0x3f, 0xa2, 0xd7, 0xc3, 0x47, 0xf3, 0x44,
  0x27, 0xf9, 0xff, 0x5c, 0x8d, 0xe6, 0xd6, 0x5d, 0xac, 0x95, 0xf2, 0xf1,
  0x9d, 0xac, 0x00, 0x53, 0xdf, 0x11, 0xa5, 0x07, 0xfb, 0x62, 0x5e, 0xeb,
  0x8d, 0xa4, 0xc0, 0x29, 0x9e, 0x4a, 0x21, 0x12, 0xab, 0x70, 0x47, 0x58,
  0x8b, 0x8d, 0x6d, 0xa7, 0x59, 0x22, 0x14, 0xf2, 0xdb, 0xa1, 0x40, 0xc7,
  0xd1, 0x22, 0x57, 0x9b, 0x5f, 0x38, 0x3d, 0x22, 0x53, 0xc8, 0xb9, 0xcb,
  0x5b, 0xc3, 0x54, 0x3a, 0x55, 0x66, 0x0b, 0xda, 0x80, 0x94, 0x6a, 0xfb,
  0x05, 0x25, 0xe8, 0xe5, 0x58, 0x6b, 0x4e, 0x63, 0xe8, 0x92, 0x41, 0x49,
  0x78, 0x36, 0xd8, 0xd3, 0xab, 0x08, 0x8c, 0xd4, 0x4c, 0x21, 0x4d, 0x6a,
  0xc8, 0x56, 0xe2, 0xa0, 0x07, 0xf4, 0x4f, 0x83, 0x74, 0x33, 0x37, 0x37,
  0x1a, 0xdd, 0x8e, 0x03, 0x00, 0x01, 0x00, 0x01, 0x42, 0x08, 0x65, 0x73,
  0x52, 0x44, 0x49, 0x60, 0x00, 0x06
};

/* Terminal (IFD) certificate in CVC format (PK.IFD.AUT) */
static u8 C_CV_IFDUser_AUT_cert [] = {
  0x7f, 0x21, 0x81, 0xcd, 0x5f, 0x37, 0x81, 0x80, 0x82, 0x5b, 0x69, 0xc6,
  0x45, 0x1e, 0x5f, 0x51, 0x70, 0x74, 0x38, 0x5f, 0x2f, 0x17, 0xd6, 0x4d,
  0xfe, 0x2e, 0x68, 0x56, 0x75, 0x67, 0x09, 0x4b, 0x57, 0xf3, 0xc5, 0x78,
  0xe8, 0x30, 0xe4, 0x25, 0x57, 0x2d, 0xe8, 0x28, 0xfa, 0xf4, 0xde, 0x1b,
  0x01, 0xc3, 0x94, 0xe3, 0x45, 0xc2, 0xfb, 0x06, 0x29, 0xa3, 0x93, 0x49,
  0x2f, 0x94, 0xf5, 0x70, 0xb0, 0x0b, 0x1d, 0x67, 0x77, 0x29, 0xf7, 0x55,
  0xd1, 0x07, 0x02, 0x2b, 0xb0, 0xa1, 0x16, 0xe1, 0xd7, 0xd7, 0x65, 0x9d,
  0xb5, 0xc4, 0xac, 0x0d, 0xde, 0xab, 0x07, 0xff, 0x04, 0x5f, 0x37, 0xb5,
  0xda, 0xf1, 0x73, 0x2b, 0x54, 0xea, 0xb2, 0x38, 0xa2, 0xce, 0x17, 0xc9,
  0x79, 0x41, 0x87, 0x75, 0x9c, 0xea, 0x9f, 0x92, 0xa1, 0x78, 0x05, 0xa2,
  0x7c, 0x10, 0x15, 0xec, 0x56, 0xcc, 0x7e, 0x47, 0x1a, 0x48, 0x8e, 0x6f,
  0x1b, 0x91, 0xf7, 0xaa, 0x5f, 0x38, 0x3c, 0xad, 0xfc, 0x12, 0xe8, 0x56,
  0xb2, 0x02, 0x34, 0x6a, 0xf8, 0x22, 0x6b, 0x1a, 0x88, 0x21, 0x37, 0xdc,
  0x3c, 0x5a, 0x57, 0xf0, 0xd2, 0x81, 0x5c, 0x1f, 0xcd, 0x4b, 0xb4, 0x6f,
  0xa9, 0x15, 0x7f, 0xdf, 0xfd, 0x79, 0xec, 0x3a, 0x10, 0xa8, 0x24, 0xcc,
  0xc1, 0xeb, 0x3c, 0xe0, 0xb6, 0xb4, 0x39, 0x6a, 0xe2, 0x36, 0x59, 0x00,
  0x16, 0xba, 0x69, 0x00, 0x01, 0x00, 0x01, 0x42, 0x08, 0x65, 0x73, 0x53,
  0x44, 0x49, 0x60, 0x00, 0x06
};

static u8 root_ca_keyref[] = { 0x02,0x0f };

static u8 cvc_intca_keyref[] = { 0x65,0x73,0x53,0x44,0x49,0x60,0x00,0x06 };

static u8 cvc_ifd_keyref[] = { 0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x01 };

static u8 icc_priv_keyref[] = { 0x02, 0x1f };

static u8 sn_ifd[] = { 0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x01 };

/************ internal functions **********************************/

/**
 * Select a file from card, process fci and read data
 * This is done by mean of iso_select_file() and iso_read_binary()
 *@param card pointer to sc_card data
 *@param path pathfile
 *@param file pointer to resulting file descriptor
 *@param buffer pointer to buffer where to store file contents
 *@param length length of buffer data
 *@return SC_SUCCESS if ok; else error code
 */
int dnie_read_file(
        sc_card_t *card,
        const sc_path_t *path,
        sc_file_t **file,
        u8 **buffer,
        size_t *length
        ) {
    u8 *data;
    char *msg=NULL;
    int res = SC_SUCCESS;
    unsigned int fsize=0; /* file size */
    if( !card || !card->ctx ) return SC_ERROR_INVALID_ARGUMENTS;
    sc_context_t *ctx= card->ctx;
    LOG_FUNC_CALLED(card->ctx);
    if (!buffer || !length || !path ) /* check received arguments */
        LOG_FUNC_RETURN(ctx,SC_ERROR_INVALID_ARGUMENTS);
    // /* try to adquire lock on card */
    // res=sc_lock(card);
    // LOG_TEST_RET(ctx, res, "sc_lock() failed");
    /* select file by mean of iso7816 ops */
    res=card->ops->select_file(card,path,file);
    if (res!=SC_SUCCESS) {
        msg="select_file failed";
        goto dnie_read_file_err;
    }
    /* iso's select file calls if needed process_fci, so arriving here
     * we have file structure filled.
     */
    if ((*file)->type==SC_FILE_TYPE_DF) {
        /* just a DF, no need to read_binary() */
        *buffer=NULL;
        *length=0;
        res=SC_SUCCESS;
        msg="File is a DF: no need to read_binary()";
        goto dnie_read_file_end;
    }
    fsize=(*file)->size;
    /* reserve enought space to read data from card*/
    if(fsize<= 0) {
        res = SC_ERROR_FILE_TOO_SMALL;
        msg="provided buffer size is too small";
        goto dnie_read_file_err;
    }
    data=calloc(fsize,sizeof(u8));
    if (data==NULL) {
        res = SC_ERROR_OUT_OF_MEMORY;
        msg="cannot reserve requested buffer size";
        goto dnie_read_file_err;
    }
    /* call sc_read_binary() to retrieve data */
    sc_log(ctx,"read_binary(): expected '%d' bytes",fsize);
    res=sc_read_binary(card,0,data,fsize,0L);
    if (res<0) { /* read_binary returns number of bytes readed */
        res = SC_ERROR_CARD_CMD_FAILED;
        msg="read_binary() failed";
        goto dnie_read_file_err;
    }
    *buffer=data;
    *length=res;
    /* now check if needed to uncompress data */
    /* TODO: dnie_read_file() check if uncompress data is required */
    /* arriving here means success */
    res=SC_SUCCESS;
    goto dnie_read_file_end;
dnie_read_file_err:
    if (*file) sc_file_free(*file);
dnie_read_file_end:
    // sc_unlock(card);
    if (msg) sc_log(ctx,msg);
    LOG_FUNC_RETURN(ctx,res);
}

static int dnie_read_certificate(sc_card_t *card, char *certpath, X509 **cert) {
    sc_file_t *file=NULL;
    sc_path_t *path=NULL;
    u8 *buffer=NULL;
    char *msg=NULL;
    size_t bufferlen=0;
    int res=SC_SUCCESS;
   
    LOG_FUNC_CALLED(card->ctx);
    path= (sc_path_t *) calloc(1,sizeof(sc_path_t));
    if (!path) {
        msg="Cannot allocate path data for cert read";
        res=SC_ERROR_OUT_OF_MEMORY;
        goto read_cert_end;
    }
    sc_format_path(certpath,path);
    res=dnie_read_file(card,path,&file,&buffer,&bufferlen);
    if (res!=SC_SUCCESS) {
        msg="Cannot get intermediate CA cert";
        goto read_cert_end;
    }
    *cert= d2i_X509(NULL,(const unsigned char **)buffer,bufferlen);
    if (*cert==NULL) { /* received data is not a certificate */
        res=SC_ERROR_OBJECT_NOT_VALID;
        msg="Readed data is not a certificate";
        goto read_cert_end;
    }
    res=SC_SUCCESS;

read_cert_end:
    if (file) { sc_file_free(file); file=NULL;buffer=NULL; bufferlen=0; }
    if (msg) sc_log(card->ctx,msg);
    LOG_FUNC_RETURN(card->ctx,res);
}

/************ implementation of cwa provider methods **************/

static int dnie_get_root_ca_pubkey( sc_card_t *card, EVP_PKEY **root_ca_key) {
    LOG_FUNC_CALLED(card->ctx);

    /* compose root_ca_public key with data provided by Dnie Manual */
    *root_ca_key= EVP_PKEY_new();
    RSA *root_ca_rsa = RSA_new();
    if ( !*root_ca_key || !root_ca_rsa ) {
        sc_log(card->ctx,"Cannot create data for root CA public key");
        return SC_ERROR_OUT_OF_MEMORY;
    } 
    root_ca_rsa->n=BN_bin2bn(icc_root_ca_modulus,
                             sizeof(icc_root_ca_modulus),
                             root_ca_rsa->n);
    root_ca_rsa->e=BN_bin2bn(icc_root_ca_public_exponent,
                             sizeof(icc_root_ca_public_exponent),
                             root_ca_rsa->e);
    int res=EVP_PKEY_assign_RSA(*root_ca_key,root_ca_rsa);
    if (!res) {
        if (*root_ca_key) EVP_PKEY_free(*root_ca_key); /*implies root_ca_rsa free()*/
        sc_log(card->ctx,"Cannot compose root CA public key");
        return SC_ERROR_INTERNAL;
    }
    LOG_FUNC_RETURN(card->ctx,SC_SUCCESS);
}

/* retrieve CVC intermediate CA certificate and length */
static int dnie_get_cvc_ca_cert(sc_card_t *card, u8 **cert, size_t *length) {
    LOG_FUNC_CALLED(card->ctx);
    *cert=C_CV_CA_CS_AUT_cert;
    *length=sizeof(C_CV_CA_CS_AUT_cert);
    LOG_FUNC_RETURN(card->ctx,SC_SUCCESS);
}

/* retrieve CVC IFD certificate and length */
static int dnie_get_cvc_ifd_cert(sc_card_t *card, u8 **cert, size_t *length) {
    LOG_FUNC_CALLED(card->ctx);
    *cert=C_CV_IFDUser_AUT_cert;
    *length=sizeof(C_CV_IFDUser_AUT_cert);
    LOG_FUNC_RETURN(card->ctx,SC_SUCCESS);
}

static int dnie_get_ifd_privkey( sc_card_t *card, EVP_PKEY **ifd_privkey) {

    LOG_FUNC_CALLED(card->ctx);

    /* compose ifd_private key with data provided in Annex 3 of DNIe Manual */
    *ifd_privkey= EVP_PKEY_new();
    RSA *ifd_rsa = RSA_new();
    if ( !*ifd_privkey || !ifd_rsa ) {
        sc_log(card->ctx,"Cannot create data for IFD private key");
        return SC_ERROR_OUT_OF_MEMORY;
    }
    ifd_rsa->n =
        BN_bin2bn(ifd_modulus,sizeof(ifd_modulus),ifd_rsa->n);
    ifd_rsa->e =
        BN_bin2bn(ifd_public_exponent,sizeof(ifd_public_exponent),ifd_rsa->e);
    ifd_rsa->d =
        BN_bin2bn(ifd_private_exponent,sizeof(ifd_private_exponent),ifd_rsa->d);
    int res=EVP_PKEY_assign_RSA(*ifd_privkey,ifd_rsa);
    if (!res) {
        if (*ifd_privkey) EVP_PKEY_free(*ifd_privkey); /* implies ifd_rsa free() */
        sc_log(card->ctx,"Cannot compose IFD private key");
        return SC_ERROR_INTERNAL;
    }
    LOG_FUNC_RETURN(card->ctx,SC_SUCCESS);
}

/* get ICC intermediate CA  path */
static int dnie_get_icc_intermediate_ca_cert(sc_card_t *card, X509 **cert){
    return dnie_read_certificate(card,"3F006020",cert);
}

/* get ICC certificate path */
static int dnie_get_icc_cert(sc_card_t *card, X509 **cert){
    return dnie_read_certificate(card,"3F00601F",cert);
}

/* Retrieve key reference for Root CA to validate CVC intermediate CA certs */
static int dnie_get_root_ca_pubkey_ref(sc_card_t *card, u8 **buf, size_t *len) {
   *buf= root_ca_keyref;
   *len= sizeof(root_ca_keyref);
   return SC_SUCCESS;
}

/* Retrieve key reference for intermediate CA to validate IFD certs */
static int dnie_get_intermediate_ca_pubkey_ref(sc_card_t *card, u8 **buf, size_t *len) {
   *buf= cvc_ifd_keyref;
   *len= sizeof(cvc_ifd_keyref);
   return SC_SUCCESS;
}

/* Retrieve key reference for IFD certificate */
static int dnie_get_ifd_pubkey_ref(sc_card_t *card, u8 **buf, size_t *len) {
   *buf= cvc_ifd_keyref;
   *len= sizeof(cvc_ifd_keyref);
   return SC_SUCCESS;
}

/* Retrieve key reference for ICC privkey */
static int dnie_get_icc_privkey_ref(sc_card_t *card, u8 **buf, size_t *len) {
   *buf= icc_priv_keyref;
   *len= sizeof(icc_priv_keyref);
   return SC_SUCCESS;
}

/* Retrieve SN.IFD */
static int dnie_get_sn_ifd(sc_card_t *card, u8 **buf, size_t *len) {
   *buf= sn_ifd;
   *len= sizeof(sn_ifd);
   return SC_SUCCESS;
}

/* Retrieve SN.ICC */
static int dnie_get_sn_icc(sc_card_t *card, sc_serial_number_t **serial) {
   return sc_card_ctl(card,SC_CARDCTL_GET_SERIALNR, *serial);
}

/* 
 * DNIe needs to get icc serial number at the begin of the sm creation
 * (to avoid breaking key references) so get it an store into serialnr 
 * cache here.
 *
 * In this way if get_sn_icc is called(), we make sure that no APDU
 * command is to be sent to card, just retrieve it from cache 
 */
/* pre secure channel creator initialization routine */
static int dnie_create_pre_ops(sc_card_t *card, cwa_provider_t *provider){
    sc_serial_number_t serial;

    /* make sure that this cwa provider is used with a working DNIe card */
    if (card->type!=SC_CARD_TYPE_DNIE_USER)
        LOG_FUNC_RETURN(card->ctx,SC_ERROR_INVALID_CARD);

    /* ensure that Card Serial Number is properly cached */
    return sc_card_ctl(card,SC_CARDCTL_GET_SERIALNR, &serial);
}

/* convert little-endian data into unsigned long */
static unsigned long le2ulong(u8 *pt) {
   unsigned long res=0;
   res = pt[0] + pt[1]<<8 + pt[2]<<16 + pt[3]<<24;
   return res;
}

/* 
 * DNIe stores some certificates in compressed format.
 * As these certificates are allways retrieved when SM is on,
 * the best way to detect and unencrypt certificates in a
 * transparent way is after an apdu decode operation
 *
 * In this way no need for any kind of file cache, just call
 * read_binary() and SM code will do the trick :-)
 */
static int dnie_decode_post_ops( 
    sc_card_t *card, 
    cwa_provider_t *provider, 
    sc_apdu_t *apdu) {

#ifdef ENABLE_ZLIB
    unsigned long compressed;
    unsigned long uncompressed;
    int res;
    u8 *upt;
    if (apdu->resplen<8) return SC_SUCCESS; /* no compression posible */
    compressed=le2ulong(apdu->resp);
    uncompressed=le2ulong(apdu->resp+4);
    if (compressed!=apdu->resplen-8) return SC_SUCCESS;/* not compressed */
    if (uncompressed<compressed) return SC_SUCCESS; /* not compressed */
    upt=calloc(uncompressed,sizeof(u8));
    if(!upt)return SC_ERROR_OUT_OF_MEMORY;
    res=sc_decompress(upt,(size_t *)uncompressed,
                      apdu->resp+8,compressed,
                      COMPRESSION_ZLIB);
    if (res!=SC_SUCCESS) {
        sc_log(card->ctx,"Uncompression failed");
        return SC_SUCCESS; /* assume that still may not need uncompression */
    }
    /* copy uncompressed data and len into apdu response */
    free(apdu->resp); /* no longer needed */
    apdu->resp=upt;
    apdu->resplen=uncompressed;
#endif

    return SC_SUCCESS;
}

cwa_provider_t *dnie_get_cwa_provider(sc_card_t *card) {

    cwa_provider_t *res=cwa_get_default_provider(card);
    if (!res) return NULL;

    /* set up proper data */

    /* pre and post operations */
    res->cwa_create_pre_ops = dnie_create_pre_ops;
    res->cwa_create_post_ops = NULL; 

    /* Get ICC intermediate CA  path */
    res->cwa_get_icc_intermediate_ca_cert = dnie_get_icc_intermediate_ca_cert;
    /* Get ICC certificate path */
    res->cwa_get_icc_cert = dnie_get_icc_cert;

    /* Obtain RSA public key from RootCA*/
    res->cwa_get_root_ca_pubkey = dnie_get_root_ca_pubkey;
    /* Obtain RSA IFD private key */
    res->cwa_get_ifd_privkey = dnie_get_ifd_privkey;

    /* Retrieve CVC intermediate CA certificate and length */
    res->cwa_get_cvc_ca_cert = dnie_get_cvc_ca_cert;
    /* Retrieve CVC IFD certificate and length */
    res->cwa_get_cvc_ifd_cert = dnie_get_cvc_ifd_cert;

    /* Get public key references for Root CA to validate intermediate CA cert */
    res->cwa_get_root_ca_pubkey_ref = dnie_get_root_ca_pubkey_ref;

    /* Get public key reference for IFD intermediate CA certificate */
    res->cwa_get_intermediate_ca_pubkey_ref = dnie_get_intermediate_ca_pubkey_ref;

    /* Get public key reference for IFD CVC certificate */
    res->cwa_get_ifd_pubkey_ref = dnie_get_ifd_pubkey_ref;

    /* Get ICC private key reference */
    res->cwa_get_icc_privkey_ref = dnie_get_icc_privkey_ref;

    /* Get IFD Serial Number */
    res->cwa_get_sn_ifd = dnie_get_sn_ifd;

    /* Get ICC Serial Number */
    res->cwa_get_sn_icc = dnie_get_sn_icc;

    /************** operations related with APDU encoding ******************/

    /* pre and post operations */
    res->cwa_encode_pre_ops = NULL;
    res->cwa_encode_post_ops = NULL;

    /************** operations related APDU response decoding **************/

    /* pre and post operations */
    res->cwa_decode_pre_ops = NULL;
    res->cwa_decode_post_ops = dnie_decode_post_ops;

    return res;
}

#endif /* HAVE_OPENSSL */
