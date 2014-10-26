#include "Stream.h"

#include "Debug.h"

#include <SwizzleManagerClass.h>

#include <Objidl.h>

bool AresByteStream::ReadFromStream(IStream *pStm, const size_t Length) {
	ULONG out = 0;
	auto size = this->Data.size();
	this->Data.resize(size + Length);
	auto pv = reinterpret_cast<void *>(this->Data.data());
	auto success = pStm->Read(pv, Length, &out);
	bool result(SUCCEEDED(success) && out == Length);
	if(!result) {
		this->Data.resize(size);
	}
	return result;
}

bool AresByteStream::WriteToStream(IStream *pStm) const {
	ULONG out = 0;
	const size_t Length(this->Data.size());
	auto pcv = reinterpret_cast<const void *>(this->Data.data());
	auto success = pStm->Write(pcv, Length, &out);
	return SUCCEEDED(success) && out == Length;
}

size_t AresByteStream::ReadBlockFromStream(IStream *pStm) {
	ULONG out = 0;
	size_t Length = 0;
	if(SUCCEEDED(pStm->Read(&Length, sizeof(Length), &out))) {
		if(this->ReadFromStream(pStm, Length)) {
			return Length;
		}
	}
	return 0;
}

bool AresByteStream::WriteBlockToStream(IStream *pStm) const {
	ULONG out = 0;
	const size_t Length = this->Data.size();
	if(SUCCEEDED(pStm->Write(&Length, sizeof(Length), &out))) {
		return this->WriteToStream(pStm);
	}
	return false;
}

bool AresStreamReader::RegisterChange(void* newPtr) {
	static_assert(sizeof(long) == sizeof(void*), "long and void* need to be of same size.");

	long oldPtr = 0;
	if(this->Load(oldPtr)) {
		if(SUCCEEDED(SwizzleManagerClass::Instance.Here_I_Am(oldPtr, newPtr))) {
			return true;
		}

		this->EmitSwizzleWarning(oldPtr, newPtr);
	}
	return false;
}

void AresStreamReader::EmitExpectEndOfBlockWarning() const {
	Debug::Log("[AresStreamReader] Read %X bytes instead of %X!\n",
		this->stream->Offset(), this->stream->Size());
}

void AresStreamReader::EmitLoadWarning(size_t size) const {
	Debug::Log("[AresStreamReader] Could not read data of length %u at %X of %X.\n",
		size, this->stream->Offset() - size, this->stream->Size());
}

void AresStreamReader::EmitExpectWarning(unsigned int found, unsigned int expect) const {
	Debug::Log("[AresStreamReader] Found %X, expected %X\n", found, expect);
}

void AresStreamReader::EmitSwizzleWarning(long id, void* pointer) const {
	Debug::Log("[AresStreamReader] Could not register change from %X to %p\n", id, pointer);
}
