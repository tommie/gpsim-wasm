#ifndef SRC_ERRORS_H_
#define SRC_ERRORS_H_

#include <stdexcept>
#include <string>

//*****************************************************************
class Error : public std::runtime_error {
public:
  explicit Error(const std::string &msg) : std::runtime_error(msg) {}
};

class FatalError : public std::runtime_error {
public:
  explicit FatalError(const std::string &msg) : std::runtime_error(msg) {}
};

//*****************************************************************
class TypeMismatch : public Error {
public:
  TypeMismatch(const std::string &theOperator, const std::string &expectedType,
    const std::string &observedType);
  TypeMismatch(const std::string &theOperator, const std::string &observedType);
  virtual ~TypeMismatch();
};

#endif // SRC_ERRORS_H_
