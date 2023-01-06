#include <cstring>
#include <stdio.h>

#include "exports.h"
#include "program_files.h"
#include "pic-processor.h"
#include "cod.h"
#include "hexutils.h"
#include "ui.h"

/**
  * RegisterProgramFileType
  * Exported function for external modules to register their
  * file types.
  */

void LIBGPSIM_EXPORT RegisterProgramFileType(ProgramFileType * pPFT)
{
  ProgramFileTypeList::GetList().push_back(pPFT);
}


void ProgramFileType::DisplayError(int err, const char *pProgFilename,
                                   const char *pLstFile)
{
  int iMessage;
  const char * pArg = "";

  switch (err) {
  case ERR_UNRECOGNIZED_PROCESSOR:
    iMessage = IDS_PROGRAM_FILE_PROCESSOR_NOT_KNOWN;
    break;

  case ERR_FILE_NOT_FOUND:
    iMessage = IDS_FILE_NOT_FOUND;
    pArg = pProgFilename;
    break;

  case ERR_FILE_NAME_TOO_LONG:
    iMessage = IDS_FILE_NAME_TOO_LONG;
    pArg = pProgFilename;
    break;

  case ERR_LST_FILE_NOT_FOUND:
    if (pLstFile == nullptr) {
      iMessage = IDS_LIST_FILE_NOT_FOUND;
      pArg = pProgFilename;

    } else {
      iMessage = IDS_FILE_NOT_FOUND;
      pArg = pLstFile;
    }

    break;

  case ERR_BAD_FILE:
    iMessage = IDS_FILE_BAD_FORMAT;
    pArg = pProgFilename;
    break;

  case ERR_NO_PROCESSOR_SPECIFIED:
    iMessage = IDS_NO_PROCESSOR_SPECIFIED;
    break;

  case ERR_PROCESSOR_INIT_FAILED:
    iMessage = IDS_PROCESSOR_INIT_FAILED;
    break;

  case ERR_NEED_PROCESSOR_SPECIFIED:
    iMessage = IDS_FILE_NEED_PROCESSOR_SPECIFIED;
    break;

  default:
    iMessage = SUCCESS;
    break;
  }

  if (iMessage != SUCCESS) {
    GetUserInterface().DisplayMessage(iMessage, pArg);
  }
}


/**
  * ProgramFileTypeList
  * Singleton class to manage the many (as of now three) file types.
  */
ProgramFileTypeList * ProgramFileTypeList::s_ProgramFileTypeList =
  new ProgramFileTypeList();
// We will instantiate g_HexFileType and g_CodFileType here to be sure
// they are instantiated after s_ProgramFileTypeList. The objects will
// move should the PIC code moved to its own external module.
static IntelHexProgramFileType g_HexFileType;
static PicCodProgramFileType g_CodFileType;



ProgramFileTypeList &ProgramFileTypeList::GetList()
{
  return *s_ProgramFileTypeList;
}


ProgramFileTypeList::ProgramFileTypeList()
{
  reserve(5);
}


ProgramFileTypeList::~ProgramFileTypeList()
{
}


bool ProgramFileTypeList::LoadProgramFile(Processor **pProcessor,
    const char *pFilename,
    FILE *pFile, const char *pProcessorName, CSimulationContext *pSimContext)
{
  iterator it;
  iterator itLast;
  iterator itEnd = end();
  int iReturn = ProgramFileType::SUCCESS;

  for (it = begin(); it != itEnd; ++it) {
    itLast = it;
    fseek(pFile, 0, SEEK_SET);

    //get_symbol_table().clear();
    if ((iReturn = (*it)->LoadProgramFile(pProcessor, pFilename, pFile, pProcessorName, pSimContext))
        == ProgramFileType::SUCCESS) {
      return true;
    }

    if (IsErrorDisplayableInLoop(iReturn)) {
      (*it)->DisplayError(iReturn, pFilename, nullptr);
    }
  }

  if (!IsErrorDisplayableInLoop(iReturn)) {
    (*itLast)->DisplayError(iReturn, pFilename, nullptr);
  }

  return false;
}


bool ProgramFileTypeList::IsErrorDisplayableInLoop(int iError)
{
  return iError != ProgramFileType::SUCCESS &&
         iError != ProgramFileType::ERR_BAD_FILE &&
         iError != ProgramFileType::ERR_NEED_PROCESSOR_SPECIFIED &&
         iError != ProgramFileType::ERR_LST_FILE_NOT_FOUND;
}


///
/// ProgramFileBuf
/// Used to wrap the FILE pointer to the program file
/// for libraries that use istreams.
/////////////////////////////////////////////////////

ProgramFileBuf::ProgramFileBuf(FILE *pFile)
{
  m_pFile = pFile;
  setg(m_Buffer + 4, m_Buffer + 4, m_Buffer + 4);
}


ProgramFileBuf::int_type ProgramFileBuf::underflow()
{
  if (gptr() < egptr()) {
    return *gptr();
  }

  int numPutback;
  numPutback = gptr() - eback();

  if (numPutback > 4) {
    numPutback = 4;
  }

  std::memcpy(m_Buffer + (4 - numPutback), gptr() - numPutback, numPutback);
  int num;

  errno = 0;
  if ((num = ::fread((void*)(m_Buffer + 4), 1, m_iBufferSize - 4, m_pFile)) <= 0) {
    if (errno != 0) {
      printf("%s\n", strerror(errno));
    }

    return (int_type)traits_type::eof();
  }

  setg(m_Buffer + (4 - numPutback), m_Buffer + 4, m_Buffer + 4 + num);
  return *gptr();
}


std::streamsize ProgramFileBuf::xsgetn(char_type *_Ptr, std::streamsize _Count)
{
  return ::fread(_Ptr, _Count, 1, m_pFile);
}
