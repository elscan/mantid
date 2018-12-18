// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/* soapserializersStub.h
   Generated by gSOAP 2.8.15 from soapserializers.h

Copyright(C) 2000-2013, Robert van Engelen, Genivia Inc. All Rights Reserved.
The generated code is released under ONE of the following licenses:
GPL or Genivia's license for commercial use.
This program is released under the GPL with the additional exemption that
compiling, linking, and/or using OpenSSL is allowed.
*/

#ifndef soapserializersStub_H
#define soapserializersStub_H
#include "stdsoap2.h"
#if GSOAP_VERSION != 20815
#error "GSOAP VERSION MISMATCH IN GENERATED CODE: PLEASE REINSTALL PACKAGE"
#endif

/******************************************************************************\
 *                                                                            *
 * Enumerations                                                               *
 *                                                                            *
\******************************************************************************/

/******************************************************************************\
 *                                                                            *
 * Types with Custom Serializers                                              *
 *                                                                            *
\******************************************************************************/

/******************************************************************************\
 *                                                                            *
 * Classes and Structs                                                        *
 *                                                                            *
\******************************************************************************/

#if 0 /* volatile type: do not declare here, declared elsewhere */

#endif

#ifndef WITH_NOGLOBAL

#ifndef SOAP_TYPE_SOAP_ENV__Header
#define SOAP_TYPE_SOAP_ENV__Header (7)
/* SOAP Header: */
struct SOAP_ENV__Header {
public:
  int soap_type() const {
    return 7;
  } /* = unique id SOAP_TYPE_SOAP_ENV__Header */
#ifdef WITH_NOEMPTYSTRUCT
private:
  char dummy; /* dummy member to enable compilation */
#endif
};
#endif

#endif

#ifndef WITH_NOGLOBAL

#ifndef SOAP_TYPE_SOAP_ENV__Code
#define SOAP_TYPE_SOAP_ENV__Code (8)
/* SOAP Fault Code: */
struct SOAP_ENV__Code {
public:
  char *SOAP_ENV__Value; /* optional element of type xsd:QName */
  struct SOAP_ENV__Code
      *SOAP_ENV__Subcode; /* optional element of type SOAP-ENV:Code */
public:
  int soap_type() const { return 8; } /* = unique id SOAP_TYPE_SOAP_ENV__Code */
};
#endif

#endif

#ifndef WITH_NOGLOBAL

#ifndef SOAP_TYPE_SOAP_ENV__Detail
#define SOAP_TYPE_SOAP_ENV__Detail (10)
/* SOAP-ENV:Detail */
struct SOAP_ENV__Detail {
public:
  char *__any;
  int __type;  /* any type of element <fault> (defined below) */
  void *fault; /* transient */
public:
  int soap_type() const {
    return 10;
  } /* = unique id SOAP_TYPE_SOAP_ENV__Detail */
};
#endif

#endif

#ifndef WITH_NOGLOBAL

#ifndef SOAP_TYPE_SOAP_ENV__Reason
#define SOAP_TYPE_SOAP_ENV__Reason (13)
/* SOAP-ENV:Reason */
struct SOAP_ENV__Reason {
public:
  char *SOAP_ENV__Text; /* optional element of type xsd:string */
public:
  int soap_type() const {
    return 13;
  } /* = unique id SOAP_TYPE_SOAP_ENV__Reason */
};
#endif

#endif

#ifndef WITH_NOGLOBAL

#ifndef SOAP_TYPE_SOAP_ENV__Fault
#define SOAP_TYPE_SOAP_ENV__Fault (14)
/* SOAP Fault: */
struct SOAP_ENV__Fault {
public:
  char *faultcode;   /* optional element of type xsd:QName */
  char *faultstring; /* optional element of type xsd:string */
  char *faultactor;  /* optional element of type xsd:string */
  struct SOAP_ENV__Detail
      *detail; /* optional element of type SOAP-ENV:Detail */
  struct SOAP_ENV__Code
      *SOAP_ENV__Code; /* optional element of type SOAP-ENV:Code */
  struct SOAP_ENV__Reason
      *SOAP_ENV__Reason; /* optional element of type SOAP-ENV:Reason */
  char *SOAP_ENV__Node;  /* optional element of type xsd:string */
  char *SOAP_ENV__Role;  /* optional element of type xsd:string */
  struct SOAP_ENV__Detail
      *SOAP_ENV__Detail; /* optional element of type SOAP-ENV:Detail */
public:
  int soap_type() const {
    return 14;
  } /* = unique id SOAP_TYPE_SOAP_ENV__Fault */
};
#endif

#endif

/******************************************************************************\
 *                                                                            *
 * Typedefs                                                                   *
 *                                                                            *
\******************************************************************************/

#ifndef SOAP_TYPE__QName
#define SOAP_TYPE__QName (5)
using _QName = char *;
#endif

#ifndef SOAP_TYPE__XML
#define SOAP_TYPE__XML (6)
using _XML = char *;
#endif

/******************************************************************************\
 *                                                                            *
 * Externals                                                                  *
 *                                                                            *
\******************************************************************************/

#endif

/* End of soapserializersStub.h */
