#pragma once

#include <fstream>

class FileDumper
{
private:
	std::ofstream of_;

public:
	FileDumper(const std::string& path): of_(path.c_str(), std::ios::binary | std::ios::out) {}
	~FileDumper() { of_._close(); }

	void dump(char* data, int size)
	{
		of_.write(data, size);
	}
};

class YUVFileDumper: public FileDumper
{
public:
	YUVFileDumper(const std::string& path) :FileDumper(path) {}
	virtual ~YUVFileDumper() {}

	void dump(char** data, int* linesize, int w, int h)
	{
		FileDumper::dump(data[0], linesize[0] * h);

		h = h >> 1;
		FileDumper::dump(data[1], linesize[1] * h);
		FileDumper::dump(data[2], linesize[2] * h);
	}
};