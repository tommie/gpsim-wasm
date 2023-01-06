
#ifndef SRC_PROGRAM_FILES_H_
#define SRC_PROGRAM_FILES_H_

#include <stdio.h>

#include <vector>

class Processor;

/**
 *  ProgramFileType base type
 */
class ProgramFileType {
public:
  enum {
    SUCCESS                       = 0,
    ERR_UNRECOGNIZED_PROCESSOR    = -1,
    ERR_FILE_NOT_FOUND            = -2,
    ERR_FILE_NAME_TOO_LONG        = -3,
    ERR_LST_FILE_NOT_FOUND        = -4,
    ERR_BAD_FILE                  = -5,
    ERR_NO_PROCESSOR_SPECIFIED    = -6,
    ERR_PROCESSOR_INIT_FAILED     = -7,
    ERR_NEED_PROCESSOR_SPECIFIED  = -8,
  };

  virtual ~ProgramFileType()
  {
  }

  /*
   *  LoadProgramFile
   *  1) Loads a program into a processor object that is passed in.
   *  2) If no processor is passed in, it will use the program file
   *  to determine processor type, allocate the processor object and
   *  load the program into the processor object.
   *  Returns: Processor object in pProcessor
   *
   */
  virtual int  LoadProgramFile(Processor **ppProcessor,
                               const char *pFilename, FILE *pFile,
                               const char *pProcessorName) = 0;
  virtual void DisplayError(int iError, const char *pProgFilename,
                            const char *pLstFile);
};


class ProgramFileTypeList : public std::vector<ProgramFileType *> {
public:
  ProgramFileTypeList();
  virtual ~ProgramFileTypeList();

  static ProgramFileTypeList &GetList();

  static ProgramFileTypeList *s_ProgramFileTypeList;
  virtual bool LoadProgramFile(Processor **pProcessor,
                               const char *pFilename, FILE *pFile,
                               const char *pProcessorName = nullptr);
  bool IsErrorDisplayableInLoop(int iError);
};


#if defined(_MSC_VER)
#include <io.h>
#endif
#include <istream>

class ProgramFileBuf : public std::streambuf {
protected:
  static const int m_iBufferSize = 1024;
  char m_Buffer[m_iBufferSize];
  FILE * m_pFile;

public:
  explicit ProgramFileBuf(FILE * pFile);

protected:
  virtual int_type underflow() override;
  virtual std::streamsize xsgetn(char_type *_Ptr, std::streamsize _Count) override;
};


class ProgramFileStream : public std::istream {
protected:
  ProgramFileBuf m_buf;

public:
  explicit ProgramFileStream(FILE * pFile) : std::istream(&m_buf),
    m_buf(pFile)
  {
  }
};

#endif
