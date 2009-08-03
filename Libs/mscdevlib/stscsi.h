// StScsi.h: interface for the CStScsi class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STSCSI_H__A23B5F2E_3804_4E89_9485_42B3E72945B3__INCLUDED_)
#define AFX_STSCSI_H__A23B5F2E_3804_4E89_9485_42B3E72945B3__INCLUDED_
#include <winioctl.h>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define ST_SENSE_DATA_SENSE_KEY_OFFSET												2
#define ST_SENSE_DATA_ASC_OFFSET													12	
#define ST_SENSE_DATA_ASCQ_OFFSET													13

#define ST_SENSE_LDRIVE_ERROR_INVALID_DRIVE_NUMBER									0x0101
#define ST_SENSE_LDRIVE_ERROR_NOT_INITIALIZED                          				0x0102
#define ST_SENSE_LDRIVE_ERROR_HARDWARE_FAILURE                         				0x0103
#define ST_SENSE_LDRIVE_ERROR_INVALID_DRIVE_TYPE                       				0x0104
#define ST_SENSE_LDRIVE_ERROR_INVALID_INFO_TYPE                        				0x0105
#define ST_SENSE_LDRIVE_ERROR_SECTOR_OUT_OF_BOUNDS                     				0x0106
#define ST_SENSE_LDRIVE_ERROR_WRITE_FAILURE                            				0x0107
#define ST_SENSE_LDRIVE_ERROR_WRITE_PROTECTED                          				0x0108
#define ST_SENSE_LDRIVE_ERROR_MULTI_WRITE_IN_PROGRESS                  				0x0109
#define ST_SENSE_LDRIVE_ERROR_MUST_HAVE_MORE_THAN_ONE_SECTOR           				0x010A
#define ST_SENSE_LDRIVE_ERROR_MULTI_WRITE_NOT_SETUP                    				0x010B
#define ST_SENSE_LDRIVE_ERROR_WRITE_ABORT                              				0x010C
#define ST_SENSE_LDRIVE_ERROR_READ_FAILURE                             				0x010D
#define ST_SENSE_LDRIVE_ERROR_MEDIA_NOT_ALLOCATED                      				0x010E
#define ST_SENSE_LDRIVE_ERROR_LOW_LEVEL_MEDIA_FORMAT_REQUIRED          				0x010F
#define ST_SENSE_LMEDIA_ERROR_HARDWARE_FAILURE                         				0x0200
#define ST_SENSE_LMEDIA_ERROR_INVALID_MEDIA_NUMBER                     				0x0201
#define ST_SENSE_LMEDIA_ERROR_MEDIA_NOT_INITIALIZED                    				0x0202 
#define ST_SENSE_LMEDIA_ERROR_MEDIA_NOT_DISCOVERED                     				0x0203 
#define ST_SENSE_LMEDIA_ERROR_INVALID_MEDIA_INFO_TYPE                  				0x0204 
#define ST_SENSE_LMEDIA_ERROR_ALLOCATION_TO_LARGE                      				0x0205 
#define ST_SENSE_LMEDIA_ERROR_MEDIA_NOT_ERASED                         				0x0206
#define ST_SENSE_LMEDIA_ERROR_MEDIA_ERASED                             				0x0207
#define ST_SENSE_LMEDIA_ERROR_NO_REGIONS_IN_MEDIA                      				0x0208     
#define ST_SENSE_LMEDIA_ERROR_DRIVES_MAX_OUT                           				0x0209 
#define ST_SENSE_LMEDIA_ERROR_MEDIA_WRITE_PROTECTED                    				0x020A 
#define ST_SENSE_LMEDIA_ERROR_INVALID_INFO_TYPE                        				0x020B 
#define ST_SENSE_LMEDIA_ERROR_BAD_BLOCKS_MAX_OUT                       				0x020C
#define ST_SENSE_LMEDIA_ERROR_NOT_ALLOCATED                            				0x020D
#define ST_SENSE_LMEDIA_ERROR_DRIVE_TYPE_NOT_SUPPORTED                              0x0210
#define ST_SENSE_NAND_ERROR_WRITE_PHYS_SECTOR_PROGRAM_FAILED           				0x0301 
#define ST_SENSE_ERROR_MULTI_WRITE_SECTOR_FIFO_OVERFLOW                				0x0302 
#define ST_SENSE_ERROR_MULTI_WRITE_SECTOR_FIFO_UNDERFLOW               				0x0303 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_LOGICAL_SECTOR_OUT_OF_BOUNDS    				0x0401
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_RBLOCK_NOT_FOUND                				0x0402
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_NO_FREE_RBLOCKS                 				0x0403 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_USECTOR_LOOKUP_INTERNAL_1       				0x0404 
#define ST_SENSE_NAND_DATA_DRIVE_SECTOR_NOT_UPDATED                    				0x0405 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_CANT_RECYCLE_USECTOR_MAP        				0x0406 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_UBLOCK_LBAS_INCONSISTENT        				0x0407 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_HSECTORIDX_IN_UBLOCK_OUT_OF_RANGE			0x0408
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_CANT_RECYCLE_UBLOCK             				0x0409
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_BBTABLE_FULL                    				0x040A 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_UPDATE_NOT_OPEN                 				0x040B 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_ADD_USECTOR_INTERNAL_1          				0x040C 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_CANT_GET_ERASED_UBLOCK          				0x040D 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_NO_ERASED_UBLOCKS               				0x040E
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_CANT_ERASE_FREE_UBLOCK          				0x040F
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_KILLUSECTOR_INTERNAL_1          				0x0410
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_KILLUSECTOR_INTERNAL_2          				0x0411
#define ST_SENSE_NAND_DATA_DRIVE_RETURN_BLOCK_NOT_UPDATED              				0x0412
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_UBLOCK_PROTECT_TABLE_FULL       				0x0413
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_UBLOCK_ALREADY_PROTECTED        				0x0414
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_UBLOCK_NOT_PROTECTED            				0x0415
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_UBLOCKPROTECT_INTERNAL_1        				0x0416
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_RECOVERUBLOCK_INTERNAL_1        				0x0417
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_RECOVERUBLOCK_INTERNAL_2        				0x0418
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_UBLOCK_NOT_IN_UTABLE            				0x0419
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_CANT_ALLOCATE_USECTORS_MAPS     				0x041A
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_CANT_INIT_DATA_REGIONS_LIST     				0x041B
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_TOO_MANY_UBLOCKS_IN_CONFIG      				0x041C 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_USECTOR_INDEX_IS_NOT_NEXT       				0x041D 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_USECTOR_ALREADY_UPDATED         				0x041E 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_BgGC_USECTOR_ERASE_TIMEDOUT     				0x041F 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_BgGC_HSECTOR_COPY_TIMEDOUT      				0x0420
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_BgGC_ALREADY_ENABLED            				0x0421
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_BgGC_HSECTOR_ERASE_TIMEDOUT     				0x0422
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_BgGC_SECTOR_NOT_UPDATED         				0x0423
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_NO_STALE_UBLOCKS                				0x0424 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_NAND_IS_READY_TIMEOUT           				0x0425 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_CANT_CLOSE_UPDATE               				0x0426 
#define ST_SENSE_NAND_DATA_DRIVE_ERROR_INVALID_LOGICAL_SECTOR                       0x0427
#define ST_SENSE_NAND_DATA_DRVIE_ERROR_INVALID_RELATIVE_SECTOR						0x0428
#define ST_SENSE_NANDHAL_ERROR_NANDTYPE_MISMATCH                       				0x0500 
#define ST_SENSE_NANDHAL_ERROR_LOOKUP_ID_FAILED                        				0x0501 
#define ST_SENSE_NANDHAL_ERROR_INIT_PORT                               				0x0502 
#define ST_SENSE_NANDHAL_ERROR_WRITE_PORT_CMD                          				0x0503 
#define ST_SENSE_NANDHAL_ERROR_WRITE_PORT_ADDR                         				0x0504 
#define ST_SENSE_NANDHAL_ERROR_READ_PORT_DATA                          				0x0505 
#define ST_SENSE_NANDHAL_ERROR_WAIT_FOR_READY_PORT                     				0x0506 
#define ST_SENSE_NANDHAL_ERROR_POWER_UP_FLASH_PADS                     				0x0507 
#define ST_SENSE_NANDHAL_ERROR_TERMINATE_PORT                          				0x0508 
#define ST_SENSE_NANDHAL_ERROR_LOCKPORT_TIMEOUT                        				0x0509 
#define ST_SENSE_NANDHAL_ERROR_LOCKNAND_TIMEOUT                        				0x050A 
#define ST_SENSE_NANDHAL_ERROR_LOCKPORT_LOCKED                         				0x050B 
#define ST_SENSE_NANDHAL_ERROR_LOCKNAND_LOCKED                         				0x050C  
#define ST_SENSE_NANDHAL_ERROR_WRITE_DATA_PORT                         				0x050D 
#define ST_SENSE_NANDHAL_ERROR_GETSTATUS_FAILED                        				0x050E 
#define ST_SENSE_NANDHAL_ERROR_WRITE_FAILED                            				0x050F  
#define ST_SENSE_NANDHAL_ERROR_READ_FAILED                             				0x0510  
#define ST_SENSE_NANDHAL_ERROR_READID1_FAILED                          				0x0511 
#define ST_SENSE_NANDHAL_ERROR_READID2_FAILED                          				0x0512 
#define ST_SENSE_NANDHAL_ERROR_READIDEXT_FAILED                        				0x0513 
#define ST_SENSE_NANDHAL_ERROR_SETNANDBUSY_FAILED                      				0x0514 
#define ST_SENSE_NANDHAL_ERROR_ASYNCWAIT_CALLBACK_ERR                  				0x0515 
#define ST_SENSE_COMPUTE_ECC_SUCCESS                                   				0x0000          
#define ST_SENSE_COMPUTE_ECC_NOT_DONE                                  				0x0516 
#define ST_SENSE_NANDHAL_ERROR_LOCKECC_TIMEOUT                         				0x0517 
#define ST_SENSE_NANDHAL_ERROR_LOCKECC_LOCKED                          				0x0518  
#define ST_SENSE_NANDHAL_ECC_ERROR_FIXED                               				0x0519 
#define ST_SENSE_NANDHAL_ECC_FIX_FAILED                                				0x051A
#define ST_SENSE_MMC_MEDIA_ERROR_DEVICE_NOT_INSERTED                                0x0600
#define ST_SENSE_MMC_MEDIA_ERROR_RESET_FAILED                                       0x0601
#define ST_SENSE_MMC_MEDIA_APP_COMMAND_FAILED                                       0x0602
#define ST_SENSE_MMC_MEDIA_ERROR_INIT_FAILED                                        0x0603
#define ST_SENSE_MMC_MEDIA_ERROR_SEND_OP_TIMEOUT                                    0x0604
#define ST_SENSE_MMC_MEDIA_READ_OCR_FAILED                                          0x0605
#define ST_SENSE_MMC_MEDIA_UNSUPPORTED_OCR_VOLTAGES                                 0x0606
#define ST_SENSE_MMC_MEDIA_READ_CSD_FAILED                                          0x0607
#define ST_SENSE_MMC_MEDIA_INVALID_CSD_VERSION                                      0x0608
#define ST_SENSE_MMC_MEDIA_READ_CID_FAILED                                          0x0609
#define ST_SENSE_MMC_MEDIA_INVALID_CID                                              0x060A
#define ST_SENSE_MMC_MMC_MEDIA_SPEC_VERSION_NOT_SUPPORTED                           0x060B
#define ST_SENSE_MMC_MMC_MEDIA_ERROR_NOT_FORMATTED                                  0x060C
#define ST_SENSE_MMC_MMC_MEDIA_ERROR_NOT_ENUMERATED                                 0x060D
#define ST_SENSE_MMC_MMC_DATA_DRIVE_ERROR_WRITE_SECTOR_FAIL                         0x0700
#define ST_SENSE_MMC_MMC_DATA_DRIVE_ERROR_INVALID_SECTOR                            0x0701
#define ST_SENSE_MMC_MMC_DATA_DRIVE_ERROR_READ_SECTOR_FAIL                          0x0702
#define ST_SENSE_MMC_MMC_DATA_DRIVE_ERROR_WRITE_PROTECTED                           0x0703
#define ST_SENSE_MMC_MMC_DATA_DRIVE_ERROR_ERASE_FAILED                              0x0704
#define ST_SENSE_MMC_MMC_HAL_ERROR_PHYSICAL_DEVICE_BLOCKED                          0x0800
#define ST_SENSE_MMC_MMC_HAL_ERROR_PHYSICAL_DEVICE_NOT_BLOCKED                      0x0801
#define ST_SENSE_MMC_MMC_HAL_ERROR_SPI_BUS_BLOCKED                                  0x0802
#define ST_SENSE_MMC_MMC_HAL_ERROR_SPI_BUS_NOT_BLOCKED                              0x0803
#define ST_SENSE_MMC_MMC_HAL_ERROR_SPI_DRIVER_INIT_FAILED                           0x0804
#define ST_SENSE_MMC_MMC_HAL_ERROR_SPI_BUS_INIT_FAILED                              0x0805
#define ST_SENSE_MMC_MMC_HAL_ERROR_NO_COMMAND_RESPONSE                              0x0810
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_START_BYTE                                   0x0811
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_WRITE_STATUS                                 0x0812
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CSD_WRITE_STATUS                             0x0813
#define ST_SENSE_MMC_MMC_HAL_ERROR_START_BYTE_TIMEOUT                               0x0820
#define ST_SENSE_MMC_MMC_HAL_ERROR_WRITE_BUSY_TIMEOUT                               0x0821
#define ST_SENSE_MMC_MMC_HAL_ERROR_CSD_WRITE_BUSY_TIMEOUT                           0x0822
#define ST_SENSE_MMC_MMC_HAL_ERROR_ERASE_BUSY_TIMEOUT                               0x0823
#define ST_SENSE_MMC_MMC_HAL_ERROR_REGISTER_READ_TIMEOUT                            0x0824
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD0                                    0x0830
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD1                                    0x0831
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD9                                    0x0832
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD10                                   0x0833
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD12                                   0x0834
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD13                                   0x0835
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD16                                   0x0836
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD17                                   0x0837
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD18                                   0x0838
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD23                                   0x0839
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD24                                   0x083A
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD25                                   0x083B
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD27                                   0x083C
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD28                                   0x083D
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD29                                   0x083E
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD30                                   0x083F
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD32                                   0x0840
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD33                                   0x0841
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD35                                   0x0842
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD36                                   0x0843
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD38                                   0x0844
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD42                                   0x0845
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD55                                   0x0846
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD56                                   0x0847
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD58                                   0x0848
#define ST_SENSE_MMC_MMC_HAL_ERROR_CMD_FAIL_CMD59                                   0x0849
#define ST_SENSE_MMC_MMC_HAL_ERROR_SD_CMD_FAIL_ACMD13                               0x0850
#define ST_SENSE_MMC_MMC_HAL_ERROR_SD_CMD_FAIL_ACMD22                               0x0851
#define ST_SENSE_MMC_MMC_HAL_ERROR_SD_CMD_FAIL_ACMD23                               0x0852
#define ST_SENSE_MMC_MMC_HAL_ERROR_SD_CMD_FAIL_ACMD41                               0x0853
#define ST_SENSE_MMC_MMC_HAL_ERROR_SD_CMD_FAIL_ACMD42                               0x0854
#define ST_SENSE_MMC_MMC_HAL_ERROR_SD_CMD_FAIL_ACMD51                               0x0855
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD0                            0x0860
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD1                            0x0861
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD9                            0x0862
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD10                           0x0863
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD12                           0x0864
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD13                           0x0865
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD16                           0x0866
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD17                           0x0867
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD18                           0x0868
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD23                           0x0869
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD24                           0x086A
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD25                           0x086B
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD27                           0x086C
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD28                           0x086D
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD29                           0x086E
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD30                           0x086F
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD32                           0x0870
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD33                           0x0871
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD35                           0x0872
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD36                           0x0873
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD38                           0x0874
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD42                           0x0875
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD55                           0x0876
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD56                           0x0877
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD58                           0x0878
#define ST_SENSE_MMC_MMC_HAL_ERROR_BAD_CMD_RESPONSE_CMD59                           0x0879
#define ST_SENSE_MMC_MMC_HAL_ERROR_SD_BAD_CMD_RESPONSE_ACMD13                       0x0880
#define ST_SENSE_MMC_MMC_HAL_ERROR_SD_BAD_CMD_RESPONSE_ACMD22                       0x0881
#define ST_SENSE_MMC_MMC_HAL_ERROR_SD_BAD_CMD_RESPONSE_ACMD23                       0x0882
#define ST_SENSE_MMC_MMC_HAL_ERROR_SD_BAD_CMD_RESPONSE_ACMD41                       0x0883
#define ST_SENSE_MMC_MMC_HAL_ERROR_SD_BAD_CMD_RESPONSE_ACMD42                       0x0884
#define ST_SENSE_MMC_MMC_HAL_ERROR_SD_BAD_CMD_RESPONSE_ACMD51                       0x0885

// whenever a new code is added increment the ST_SENSE_NUM_CODES value. Add the new code above this line.
#define ST_SENSE_NUM_CODES															199	


#include "StSDisk.h"

class CStDdiApi;																		 
																						 
class CStScsi : public CStBase															 
{																						 
																						 
public:

	CStScsi(CStUpdater* pUpdater, string name="CStScsi");
	virtual ~CStScsi();

	CStUpdater* GetUpdater(); 
	wstring GetSenseData();
	wchar_t	GetDriveLetter() { return m_drive_letter; }
    void ClearHandle() { m_handle = INVALID_HANDLE_VALUE; }
	HANDLE GetHandle() { return m_handle; }
	virtual ST_ERROR Open()=0;
	virtual ST_ERROR Lock(BOOL _media_new)=0;
	virtual ST_ERROR AcquireFormatLock(BOOL _media_new)=0;
	virtual ST_ERROR ReleaseFormatLock(BOOL _media_new)=0;
	virtual ST_ERROR Unlock(BOOL _media_new)=0;
	virtual ST_ERROR Close()=0;
	virtual ST_ERROR Dismount()=0;
	virtual ST_ERROR WriteSector( CStByteArray* _p_sector, ULONG _num_sectors, ULONG _start_sector_number, ULONG _sector_size );
	virtual ST_ERROR ReadSector( CStByteArray* _p_sector, ULONG _num_sectors, ULONG _start_sector_number, ULONG _sector_size );
	virtual ST_ERROR FormatPartition( PDISK_GEOMETRY _p_dg, ULONG _hidden_sectors );
	virtual ST_ERROR ReadGeometry( PDISK_GEOMETRY _p_dg )=0;
	virtual ST_ERROR DriveLayout( PDRIVE_LAYOUT_INFORMATION _p_dl );
	virtual ST_ERROR OpenPhysicalDrive( USHORT driveToFind, USHORT UpgradeOrNormal );

protected:

	wchar_t		    	m_drive_letter;
	SCSI_STATE  		m_state;
	HANDLE			    m_handle;
	CStByteArray*	    m_p_arr_sense_data;

private:

	CStUpdater*	m_p_updater;

public:

    UCHAR               m_ProtocolVersion;  // same as m_ProtocolVersionMajor
	UCHAR				m_ProtocolVersionMajor;
	UCHAR				m_ProtocolVersionMinor;

	SCSI_STATE GetState();

	virtual ST_ERROR Initialize(USHORT driveToFind, USHORT UpgradeOrNormal)=0;
//	virtual ST_ERROR LockLogicalVolume()=0;
//	virtual ST_ERROR UnLockLogicalVolume()=0;
	virtual ST_ERROR SendDdiApiCommand(CStDdiApi* pApi);
			
protected:

	virtual ST_ERROR SendCommand( CStByteArray* p_command_arr, UCHAR cdb_len, BOOL direction_out, 
							 CStByteArray& response_arr, ULONG ulTimeOut)=0;

	void SaveSenseData(UCHAR* _sense_data, UCHAR _sense_len);

	ST_ERROR IsSystemMedia(ST_BOOLEAN&);
    ST_ERROR GetProtocolVersionMajor( UCHAR&  );
    ST_ERROR GetProtocolVersionMinor( UCHAR&  );

};

#endif // !defined(AFX_STSCSI_H__A23B5F2E_3804_4E89_9485_42B3E72945B3__INCLUDED_)
