#ifndef NTFSMFT_H
#define NTFSMFT_H

#include <windows.h>
#include <cstdint>

#pragma pack(push, 1)

struct MFT_RECORD_HEADER {
    uint32_t magic; // "FILE" or "BAAD"
    uint16_t updateSequenceOffset;
    uint16_t updateSequenceSize; // in words
    uint64_t logFileSeqNumber;
    uint16_t sequenceNumber;
    uint16_t hardLinkCount;
    uint16_t firstAttributeOffset;
    uint16_t flags;
    uint32_t usedSize;
    uint32_t allocatedSize;
    uint64_t baseFileRecord;
    uint16_t nextAttributeId;
    uint16_t align;
    uint32_t mftRecordNumber;
};

struct ATTRIBUTE_HEADER {
    uint32_t type;
    uint32_t length;
    uint8_t nonResident;
    uint8_t nameLength;
    uint16_t nameOffset;
    uint16_t flags;
    uint16_t attributeId;
};

struct RESIDENT_ATTRIBUTE_HEADER {
    ATTRIBUTE_HEADER header;
    uint32_t valueLength;
    uint16_t valueOffset;
    uint8_t indexedFlag;
    uint8_t padding;
};

struct NON_RESIDENT_ATTRIBUTE_HEADER {
    ATTRIBUTE_HEADER header;
    uint64_t startingVcn;
    uint64_t lastVcn;
    uint16_t dataRunOffset;
    uint16_t compressionUnitSize;
    uint32_t padding;
    uint64_t allocatedSize;
    uint64_t realSize;
    uint64_t initializedSize;
};

struct STANDARD_INFORMATION {
    uint64_t creationTime;
    uint64_t lastModificationTime;
    uint64_t lastChangeTime;
    uint64_t lastAccessTime;
    uint32_t fileAttributes;
    uint32_t maxVersions;
    uint32_t versionNumber;
    uint32_t classId;
    uint32_t ownerId;
    uint32_t securityId;
    uint64_t quotaCharged;
    uint64_t usn;
};

struct FILE_NAME_ATTRIBUTE {
    uint64_t parentDirectory; // Lower 48 bits are FRN, upper 16 are sequence number
    uint64_t creationTime;
    uint64_t lastModificationTime;
    uint64_t lastChangeTime;
    uint64_t lastAccessTime;
    uint64_t allocatedSize;
    uint64_t realSize;
    uint32_t fileAttributes;
    uint32_t repaseTag;
    uint8_t nameLength;
    uint8_t nameType;
    wchar_t name[1];
};

#pragma pack(pop)

#endif // NTFSMFT_H
