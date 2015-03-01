#include "FileStream.hpp"

#include <cstring>

namespace warped {

//////////////////////////////// Constructors ///////////////////////////////////////////}

FileStream::FileStream(const std::string& filename, std::ios_base::openmode mode) {
    fstream_.open(filename, mode);
}

FileStream::FileStream (FileStream&& x) {
    *this = std::move(x);
}

FileStream& FileStream::operator= (FileStream&& rhs) {
    *this = std::move(rhs);
    return *this;
}

///////////////////////////// fstream operations //////////////////////////////////////////////

void FileStream::open(const char* filename, std::ios_base::openmode mode) {
    fstream_.open(filename, mode);
}

void FileStream::open(const std::string& filename, std::ios_base::openmode mode) {
    fstream_.open(filename, mode);
}

bool FileStream::is_open() const {
    return fstream_.is_open();
}

void FileStream::close() {
    fstream_.close();
}

std::filebuf* FileStream::rdbuf() const {
    return fstream_.rdbuf();
}

//////////////////////////// Output ////////////////////////////////////////////////////////

FileStream& FileStream::operator<< (bool val) {
    fstream_ << val;
    return *this;
}

FileStream& FileStream::operator<< (short val) {
    fstream_ << val;
    return *this;
}

FileStream& FileStream::operator<< (unsigned short val) {
    fstream_ << val;
    return *this;
}

FileStream& FileStream::operator<< (int val) {
    fstream_ << val;
    return *this;
}

FileStream& FileStream::operator<< (unsigned int val) {
    fstream_ << val;
    return *this;
}

FileStream& FileStream::operator<< (long val) {
    fstream_ << val;
    return *this;
}

FileStream& FileStream::operator<< (unsigned long val) {
    fstream_ << val;
    return *this;
}

FileStream& FileStream::operator<< (long long val) {
    fstream_ << val;
    return *this;
}

FileStream& FileStream::operator<< (unsigned long long val) {
    fstream_ << val;
    return *this;
}

FileStream& FileStream::operator<< (float val) {
    fstream_ << val;
    return *this;
}

FileStream& FileStream::operator<< (double val) {
    fstream_ << val;
    return *this;
}

FileStream& FileStream::operator<< (long double val) {
    fstream_ << val;
    return *this;
}

FileStream& FileStream::operator<< (void* val) {
    fstream_ << val;
    return *this;
}

FileStream& FileStream::operator<< (const char * val) {
    fstream_ << val;
    return *this;
}

FileStream& FileStream::operator<< (std::streambuf* sb) {
    fstream_ << sb;
    return *this;
}

FileStream& FileStream::operator<< (FileStream& (*pf)(FileStream&)) {
    *this << pf;
    return *this;
}

FileStream& FileStream::operator<< (std::ios& (*pf)(std::ios&)) {
    fstream_ << pf;
    return *this;
}

FileStream& FileStream::operator<< (std::ios_base& (*pf)(std::ios_base&)) {
    fstream_ << pf;
    return *this;
}

FileStream& FileStream::put(char c) {
    fstream_.put(c);
    return *this;
}

FileStream& FileStream::write(const char* s, std::streamsize n) {
    fstream_.write(s, n);
    return *this;
}

//////////////////////////////// Input /////////////////////////////////////////////////

FileStream& FileStream::operator>> (bool& val) {
    fstream_ >> val;
    return *this;
}

FileStream& FileStream::operator>> (short& val) {
    fstream_ >> val;
    return *this;
}

FileStream& FileStream::operator>> (unsigned short & val) {
    fstream_ >> val;
    return *this;
}

FileStream& FileStream::operator>> (int& val) {
    fstream_ >> val;
    return *this;
}

FileStream& FileStream::operator>> (unsigned int& val) {
    fstream_ >> val;
    return *this;
}

FileStream& FileStream::operator>> (long& val) {
    fstream_ >> val;
    return *this;
}

FileStream& FileStream::operator>> (unsigned long& val) {
    fstream_ >> val;
    return *this;
}

FileStream& FileStream::operator>> (float& val) {
    fstream_ >> val;
    return *this;
}

FileStream& FileStream::operator>> (double& val) {
    fstream_ >> val;
    return *this;
}

FileStream& FileStream::operator>> (long double& val) {
    fstream_ >> val;
    return *this;
}

FileStream& FileStream::FileStream::operator>> (void*& val) {
    fstream_ >> val;
    return *this;
}

FileStream& FileStream::operator>> (std::streambuf* sb) {
    fstream_ >> sb;
    return *this;
}

FileStream& FileStream::operator>> (FileStream& (*pf)(FileStream)) {
    *this >> pf;
    return *this;
}

FileStream& FileStream::operator>> (std::ios& (*pf)(std::ios&)) {
    fstream_ >> pf;
    return *this;
}

FileStream& FileStream::operator>> (std::ios_base& (*pf)(std::ios_base&)) {
    fstream_ >> pf;
    return *this;
}

std::streamsize FileStream::gcount() const {
    return fstream_.gcount();
}

int FileStream::get() {
    return fstream_.get();
}

FileStream& FileStream::get (char& c) {
    fstream_.get(c);
    return *this;
}

FileStream& FileStream::get (char* s, std::streamsize n) {
    fstream_.get(s, n);
    return *this;
}

FileStream& FileStream::get (char* s, std::streamsize n, char delim) {
    fstream_.get(s, n, delim);
    return *this;
}

FileStream& FileStream::get (std::streambuf& sb) {
    fstream_.get(sb);
    return *this;
}

FileStream& FileStream::get (std::streambuf& sb, char delim) {
    fstream_.get(sb, delim);
    return *this;
}

FileStream& FileStream::getline (char* s, std::streamsize n ) {
    fstream_.getline(s, n);
    return *this;
}

FileStream& FileStream::getline (char* s, std::streamsize n, char delim ) {
    fstream_.getline(s, n, delim);
    return *this;
}

FileStream& FileStream::ignore (std::streamsize n, int delim) {
    fstream_.ignore(n, delim);
    return *this;
}

int FileStream::peek() {
    return fstream_.peek();
}

FileStream& FileStream::read (char* s, std::streamsize n) {
    fstream_.read(s, n);
    return *this;
}

std::streamsize FileStream::readsome (char* s, std::streamsize n) {
    return fstream_.readsome(s, n);
}

FileStream& FileStream::putback (char c) {
    fstream_.putback(c);
    return *this;
}

FileStream& FileStream::unget() {
    fstream_.unget();
    return *this;
}

std::streampos FileStream::tellg() {
    return fstream_.tellg();
}

FileStream& FileStream::seekg (std::streampos pos) {
    fstream_.seekg(pos);
    return *this;
}

FileStream& FileStream::seekg (std::streamoff off, std::ios_base::seekdir way) {
    fstream_.seekg(off, way);
    return *this;
}

int FileStream::sync() {
    return fstream_.sync();
}

} // namespace warped

