/*
 * Copyright 2020 NXP.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the NXP Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

//ref: https://docs.microsoft.com/en-us/windows-hardware/manufacture/mobile/ffu-image-format

#ifndef _LIBSPARSE_FFU_FORMAT_H_
#define _LIBSPARSE_FFU_FORMAT_H_


#define FFU_SECURITY_SIGNATURE "SignedImage "

#pragma pack(1)

typedef struct _FFU_SECURITY_HEADER
{
	uint32_t cbSize;            // size of struct, overall
	uint8_t  signature[12];     // "SignedImage "
	uint32_t dwChunkSizeInKb;   // size of a hashed chunk within the image
	uint32_t dwAlgId;           // algorithm used to hash
	uint32_t dwCatalogSize;     // size of catalog to validate
	uint32_t dwHashTableSize;   // size of hash table
} FFU_SECURITY_HEADER;

#define FFU_SIGNATURE "ImageFlash  "

typedef struct _IMAGE_HEADER
{
	uint32_t  cbSize;           // sizeof(ImageHeader)
	uint8_t   Signature[12];    // "ImageFlash  "
	uint32_t  ManifestLength;   // in bytes
	uint32_t  dwChunkSize;      // Used only during image generation.
} FFU_IMAGE_HEADER;

typedef struct _STORE_HEADER
{
	uint32_t dwUpdateType; // indicates partial or full flash
	uint16_t MajorVersion, MinorVersion; // used to validate struct
	uint16_t FullFlashMajorVersion, FullFlashMinorVersion; // FFU version, i.e. the image format
	uint8_t szPlatformId[192]; // string which indicates what device this FFU is intended to be written to
	uint32_t dwBlockSizeInBytes; // size of an image block in bytes ¨C the device¡¯s actual sector size may differ
	uint32_t dwWriteDescriptorCount; // number of write descriptors to iterate through
	uint32_t dwWriteDescriptorLength; // total size of all the write descriptors, in bytes (included so they can be read out up front and interpreted later)
	uint32_t dwValidateDescriptorCount; // number of validation descriptors to check
	uint32_t dwValidateDescriptorLength; // total size of all the validation descriptors, in bytes
	uint32_t dwInitialTableIndex; // block index in the payload of the initial (invalid) GPT
	uint32_t dwInitialTableCount; // count of blocks for the initial GPT, i.e. the GPT spans blockArray[idx..(idx + count -1)]
	uint32_t dwFlashOnlyTableIndex; // first block index in the payload of the flash-only GPT (included so safe flashing can be accomplished)
	uint32_t dwFlashOnlyTableCount; // count of blocks in the flash-only GPT
	uint32_t dwFinalTableIndex; // index in the table of the real GPT
	uint32_t dwFinalTableCount; // number of blocks in the real GPT
	uint16_t NumOfStores; // Total number of stores (V2 only)
	uint16_t StoreIndex; // Current store index, 1-based (V2 only)
	uint64_t StorePayloadSize; // Payload data only, excludes padding (V2 only)
	uint16_t DevicePathLength; // Length of the device path (V2 only)
	uint16_t DevicePath[1]; // Device path has no NUL at then end (V2 only)
} FFU_STORE_HEADER;

typedef struct _VALIDATION_ENTRY
{
	uint32_t dwSectorIndex;
	uint32_t dwSectorOffset;
	uint32_t dwByteCount;
	uint8_t rgCompareData[1]; // size is dwByteCount
} FFU_VALIDATION_ENTRY;

enum DISK_ACCESS_METHOD
{
	DISK_BEGIN = 0,
	DISK_END = 2
};

typedef struct _DISK_LOCATION
{
	uint32_t dwDiskAccessMethod;
	uint32_t dwBlockIndex;
} FFU_DISK_LOCATION;

typedef struct _BLOCK_DATA_ENTRY
{
	uint32_t dwLocationCount;
	uint32_t dwBlockCount;
	FFU_DISK_LOCATION rgDiskLocations[1];
} FFU_BLOCK_DATA_ENTRY;
#pragma pack()

#endif
