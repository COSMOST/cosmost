#ifndef __STUDY_UTILS_EXCEPTION_H__
#define __STUDY_UTILS_EXCEPTION_H__

#include <exception>
#include <string>
#include <errno.h>

#include <utils/log.h>

namespace Utils {

	class Exception : public std::exception {
		protected:
			const std::string _message;
			int _code;

		public:
			explicit Exception(const char *msg) :
						_message(msg), _code(errno) {
				ERROR("%s: %s\n", strerror(errno ? errno : EIO),
									msg ? msg : "Unknown");
			}

			virtual ~Exception() throw() {}
	
			virtual const char* what() const throw() {
				return _message.c_str();
			}

			int errcode() const {
				return _code;
			}
	};
}

#endif /* __STUDY_UTILS_EXCEPTION_H__ */
