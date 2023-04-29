#include "std.h"

#if 0
typedef int (__cdecl *GetASPI32SupportInfo_t)();
typedef int (__cdecl *SendASPI32Command_t)(void *SRB);
const ATAPI_CDB_SIZE = 12; // sizeof(CDB) == 16
const MAX_INFO_LEN = 48;

GetASPI32SupportInfo_t _GetASPI32SupportInfo = 0;
SendASPI32Command_t _SendASPI32Command = 0;
HMODULE hAspiDll = 0;
HANDLE hASPICompletionEvent;

void init_aspi()
{
   if (_SendASPI32Command) return;
   hAspiDll = LoadLibrary("WNASPI32.DLL");
   if (!hAspiDll) { errmsg("failed to load WNASPI32.DLL"); err_win32(); exit(); }
   _GetASPI32SupportInfo = (GetASPI32SupportInfo_t)GetProcAddress(hAspiDll, "GetASPI32SupportInfo");
   _SendASPI32Command = (SendASPI32Command_t)GetProcAddress(hAspiDll, "SendASPI32Command");
   if (!_GetASPI32SupportInfo || !_SendASPI32Command) errexit("invalid ASPI32 library");
   DWORD init = _GetASPI32SupportInfo();
   if (((init >> 8) & 0xFF) != SS_COMP) errexit("error in ASPI32 initialization");
   hASPICompletionEvent = CreateEvent(0,0,0,0);
}

int SEND_ASPI_CMD(int adapter_id, int read_id, CDB *cdb, int CDB_sz, u8 *sense, u8 *buf, int buf_sz, int flag)
{
   SRB_ExecSCSICmd SRB = { 0 };
   SRB.SRB_Cmd        = SC_EXEC_SCSI_CMD;
   SRB.SRB_HaId       = (u8)adapter_id;
   SRB.SRB_Flags      = flag | SRB_EVENT_NOTIFY;
   SRB.SRB_Target     = read_id;
   SRB.SRB_BufPointer = buf;
   SRB.SRB_BufLen     = buf_sz;
   SRB.SRB_SenseLen   = sizeof(SRB.SenseArea);
   SRB.SRB_CDBLen     = (u8)CDB_sz;
   SRB.SRB_PostProc   = hASPICompletionEvent;
   memcpy(SRB.CDBByte, cdb, CDB_sz);

   /* DWORD ASPIStatus = */ _SendASPI32Command(&SRB);

   if (SRB.SRB_Status == SS_PENDING) {
      DWORD ASPIEventStatus = WaitForSingleObject(hASPICompletionEvent, 10000); // timeout 10sec
      if (ASPIEventStatus == WAIT_OBJECT_0) ResetEvent(hASPICompletionEvent);
   }
   if (sense) memcpy(sense, SRB.SenseArea, sizeof(SRB.SenseArea));
   return SRB.SRB_Status;
}

int SEND_ASPI_INQR(int adapter_id, int read_id, CDB *cdb, int CDB_sz, u8 *sense, u8 *buf, int buf_sz, int flag)
{
   SRB_ExecSCSICmd SRB = { 0 };
   SRB.SRB_Cmd        = SC_EXEC_SCSI_CMD;
   SRB.SRB_HaId       = (u8)adapter_id;
   SRB.SRB_Flags      = flag | SRB_EVENT_NOTIFY;
   SRB.SRB_Target     = read_id;
   SRB.SRB_BufPointer = buf;
   SRB.SRB_BufLen     = buf_sz;
   SRB.SRB_SenseLen   = sizeof(SRB.SenseArea);
   SRB.SRB_CDBLen     = (u8)CDB_sz;
   SRB.SRB_PostProc   = hASPICompletionEvent;
   memcpy(SRB.CDBByte, cdb, CDB_sz);

   /* DWORD ASPIStatus = */ _SendASPI32Command(&SRB);

   if (SRB.SRB_Status == SS_PENDING) {
      DWORD ASPIEventStatus = WaitForSingleObject(hASPICompletionEvent, 10000); // timeout 10sec
      if (ASPIEventStatus == WAIT_OBJECT_0) ResetEvent(hASPICompletionEvent);
   }
   if (sense) memcpy(sense, SRB.SenseArea, sizeof(SRB.SenseArea));
   return SRB.SRB_Status;
}


void done_aspi()
{
   if (!hAspiDll) return;
   FreeLibrary(hAspiDll);
   CloseHandle(hASPICompletionEvent);
}
#endif
