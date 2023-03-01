#ifndef STATUS
#define STATUS

#include <string>
#include <iostream>

namespace lodge {
    enum Code {
        kOk = 0,
        kNotFound = 1,
        kThreadError = 2,
    };

    struct Status {
        Code _status;
        
        Status() {
            _status = kOk;
        }

        Status(const Status& p) {
            _status = p._status;
        }

        Status& operator=(const Status& p) {
            this->_status = p._status;

            return *this;
        }

        bool ok() {
            return _status == kOk;
        }

        std::string errMessage() {
            switch (_status)
            {
            case kNotFound:
                return std::string("Key-value pair not found!");
            
            case kThreadError:
                return std::string("Multithread error!");

            default:
                break;
            }
        }


    };


}


#endif