/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_SERIALIZABLE_STRUCT_H_
#define COMMONAPI_SERIALIZABLE_STRUCT_H_

#include <cstdint>
#include <memory>


namespace CommonAPI {

class InputStream;
class OutputStream;
class TypeOutputStream;

struct SerializableStruct {
	virtual ~SerializableStruct() { }

	virtual void readFromInputStream(InputStream& inputStream) = 0;
	virtual void writeToOutputStream(OutputStream& outputStream) const = 0;
};

struct SerializablePolymorphicStruct: SerializableStruct {
	virtual uint32_t getSerialId() const = 0;
	virtual void createTypeSignature(TypeOutputStream& typeOutputStream) const = 0;
};

} // namespace CommonAPI

#endif // COMMONAPI_SERIALIZABLE_STRUCT_H_
