/*
 * errors.c: The textual representation of errors
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

#include <stdio.h>
#include "errors.h"

#define DIM(v)		(sizeof(v)/(sizeof((v)[0])))

const char *sc_strerror(int error)
{
	const char *rdr_errors[] = {
		"Generic reader error",
		"No readers found",
		"Slot not found",
		"Slot already connected",
		"Card not present",
		"Card removed",
		"Card reset",
		"Transmit failed",
		"Timed out while waiting for user input",
		"Input operation cancelled by user",
		"The two PINs did not match",
		"Message too long (keypad)",
		"Timeout while waiting for event from card reader",
		"Unresponsive card (correctly inserted?)",
		"Reader detached (hotplug device?)",
		"Reader reattached (hotplug device?)",
	};
	const int rdr_base = -SC_ERROR_READER;
	const char *card_errors[] = {
		"Card command failed",
		"File not found",
		"Record not found",
		"Unsupported CLA byte in APDU",
		"Unsupported INS byte in APDU",
		"Incorrect parameters in APDU",
		"Wrong length",
		"Card memory failure",
		"Card does not support the requested operation",
		"Not allowed",
		"Card is invalid or cannot be handled",
		"Security status not satisfied",
		"Authentication method blocked",
		"Unknown data received from card",
		"PIN code or key incorrect",
		"File already exists",
		"Data object not found",
	};
	const int card_base = -SC_ERROR_CARD_CMD_FAILED;
	const char *arg_errors[] = {
		"Invalid arguments",
		"Command too short",
		"Command too long",
		"Buffer too small",
		"Invalid PIN length",
		"Invalid data",
	};
	const int arg_base = -SC_ERROR_INVALID_ARGUMENTS;
	const char *int_errors[] = {
		"Internal error",
		"Invalid ASN.1 object",
		"Required ASN.1 object not found",
		"Premature end of ASN.1 stream",
		"Out of memory",
		"Object not valid",
		"Object not found",
		"Requested object not found",
		"Not supported",
		"Passphrase required",
		"The key is extractable",
		"Decryption failed",
		"Wrong padding",
		"Unsupported card",
		"Unable to load external module",
		"EF offset too large"
	};
	const int int_base = -SC_ERROR_INTERNAL;
	const char *p15i_errors[] = {
		"Generic PKCS #15 initialization error",
		"Syntax error",
		"Inconsistent or incomplete pkcs15 profile",
		"Key length/algorithm not supported by card",
		"No default (transport) key available",
		"The PKCS#15 Key/certificate ID specified is not unique",
		"Unable to load key and certificate(s) from file",
		"Object is not compatible with intended use",
		"File template not found",
		"Invalid PIN reference",
		"File too small",
	};
	const int p15i_base = -SC_ERROR_PKCS15INIT;
	const char *misc_errors[] = {
		"Unknown error",
		"PKCS#15 compatible smart card not found",
	};
	const int misc_base = -SC_ERROR_UNKNOWN;
	const char **errors = NULL;
	int count = 0, err_base = 0;
	
	if (error < 0)
		error = -error;
	if (error >= misc_base) {
		errors = misc_errors;
		count = DIM(misc_errors);
		err_base = misc_base;
	} else if (error >= p15i_base) {
		errors = p15i_errors;
		count = DIM(p15i_errors);
		err_base = p15i_base;
	} else if (error >= int_base) {
		errors = int_errors;
		count = DIM(int_errors);
		err_base = int_base;
	} else if (error >= arg_base) {
		errors = arg_errors;
		count = DIM(arg_errors);
		err_base = arg_base;
	} else if (error >= card_base) {
		errors = card_errors;
		count = DIM(card_errors);
		err_base = card_base;
	} else if (error >= rdr_base) {
		errors = rdr_errors;
		count = DIM(rdr_errors);
		err_base = rdr_base;
	}
	error -= err_base;
	if (error >= count || count == 0)
		return misc_errors[0];
	return errors[error];
}
