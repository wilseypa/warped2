#ifndef FILE_STREAM
#define FILE_STREAM

#include <fstream>
#include <sstream>
#include <map>

namespace warped {

class FileStream {
public:
    // Constructors
    FileStream() = default;
    explicit FileStream(const std::string& filename, std::ios_base::openmode mode);
    FileStream (const FileStream&) = delete;
    FileStream (FileStream&& x);

    virtual ~FileStream() = default;

    FileStream& operator= (const FileStream&) = delete;
    FileStream& operator= (FileStream&& rhs);

    // File operations
    void open(const char* filename, std::ios_base::openmode mode);
    void open(const std::string& filename, std::ios_base::openmode mode);
    bool is_open() const;
    void close();
    std::filebuf* rdbuf() const;

    // Output operations
    virtual FileStream& operator<< (bool val);
    virtual FileStream& operator<< (short val);
    virtual FileStream& operator<< (unsigned short val);
    virtual FileStream& operator<< (int val);
    virtual FileStream& operator<< (unsigned int val);
    virtual FileStream& operator<< (long val);
    virtual FileStream& operator<< (unsigned long val);
    virtual FileStream& operator<< (long long val);
    virtual FileStream& operator<< (unsigned long long val);
    virtual FileStream& operator<< (float val);
    virtual FileStream& operator<< (double val);
    virtual FileStream& operator<< (long double val);
    virtual FileStream& operator<< (void* val);
    virtual FileStream& operator<< (std::streambuf* sb);
    virtual FileStream& operator<< (FileStream& (*pf)(FileStream&));
    virtual FileStream& operator<< (std::ios& (*pf)(std::ios&));
    virtual FileStream& operator<< (std::ios_base& (*pf)(std::ios_base&));
    virtual FileStream& put(char c);
    virtual FileStream& write(const char* s, std::streamsize n);

    // Input operations
    FileStream& operator>> (bool& val);
    FileStream& operator>> (short& val);
    FileStream& operator>> (unsigned short & val);
    FileStream& operator>> (int& val);
    FileStream& operator>> (unsigned int& val);
    FileStream& operator>> (long& val);
    FileStream& operator>> (unsigned long& val);
    FileStream& operator>> (float& val);
    FileStream& operator>> (double& val);
    FileStream& operator>> (long double& val);
    FileStream& operator>> (void*& val);
    FileStream& operator>> (std::streambuf* sb);
    FileStream& operator>> (FileStream& (*pf)(FileStream));
    FileStream& operator>> (std::ios& (*pf)(std::ios&));
    FileStream& operator>> (std::ios_base& (*pf)(std::ios_base&));
    std::streamsize gcount() const;
    int get();
    FileStream& get (char& c);
    FileStream& get (char* s, std::streamsize n);
    FileStream& get (char* s, std::streamsize n, char delim);
    FileStream& get (std::streambuf& sb);
    FileStream& get (std::streambuf& sb, char delim);
    FileStream& getline (char* s, std::streamsize n );
    FileStream& getline (char* s, std::streamsize n, char delim );
    FileStream& ignore (std::streamsize n = 1, int delim = EOF);
    int peek();
    FileStream& read (char* s, std::streamsize n);
    std::streamsize readsome (char* s, std::streamsize n);
    FileStream& putback (char c);
    FileStream& unget();
    std::streampos tellg();
    FileStream& seekg (std::streampos pos);
    FileStream& seekg (std::streamoff off, std::ios_base::seekdir way);
    int sync();

protected:
    std::fstream fstream_;

};

struct FileStreamDeleter {
    void operator()(FileStream* p) const {
        p->close();
        delete p;
    }
};

} // namespace warped

#endif
