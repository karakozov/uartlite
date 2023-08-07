#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <vector>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>

//-----------------------------------------------------------------------------

bool get_sections(const std::string& fname, std::vector<std::string>& sections);
bool get_options(const std::string& fname, std::vector<std::string>& optionsList);
bool get_options(const std::string& fname, const std::string& section, std::vector<std::string>& optionsList);
bool check_string_format(const std::string &s);
void lowercase(std::string &s);
unsigned digit_number(unsigned data_size);
bool is_option(int argc, char **argv, const char* name);

//------------------------------------------------------------------------------

template <typename T> std::string toString(T val)
{
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

//------------------------------------------------------------------------------

template <typename T> std::string toStringHex(T val)
{
	std::ostringstream oss;
	oss << std::hex << val;
	return oss.str();
}

//------------------------------------------------------------------------------

template<typename T> T fromString(const std::string& s)
{
    std::istringstream iss(s);
    T res;
	
	if (strstr(s.c_str(), "0x")) {
        iss >> std::hex >> res;
    } else {
        iss >> res;
    }
    return res;
}

//------------------------------------------------------------------------------

template<typename T> bool get_value(const std::vector<std::string>& optionsList, unsigned& start_index, const std::string& key, T& val)
{
    if(optionsList.empty() || (start_index >= optionsList.size()))
        return false;

    bool found = false;

    for(unsigned i=start_index; i<optionsList.size(); i++) {

        const std::string& opt = optionsList.at(i);
        int begin = opt.find_first_of("=", 0);
        int end = opt.length();

        if(!begin && (begin >= end))
            continue;

        std::string value = opt.substr(++begin, end);
        if(value.empty())
            continue;

        if(strstr(opt.c_str(),key.c_str())) {
            val = fromString<T>(value);
            found = true;
			start_index = i+1;
            break;
        }
    }

    return found;
}

//-----------------------------------------------------------------------------

template<typename T> bool get_raw_value(const std::vector<std::string>& optionsList, unsigned& start_index, const std::string& key, T& val)
{
	if (optionsList.empty() || (start_index >= optionsList.size()))
		return false;

	bool found = false;

	for (unsigned i = start_index; i<optionsList.size(); i++) {

		const std::string& opt = optionsList.at(i);
		int begin = opt.find_first_of("=", 0);
		int end = opt.length();

		if (!begin && (begin >= end))
			continue;

		std::string value = opt.substr(++begin, end);
		if (value.empty())
			continue;

		if (strstr(opt.c_str(), key.c_str())) {
			val = value;
			found = true;
			start_index = i + 1;
			break;
		}
	}

	return found;
}

//-----------------------------------------------------------------------------

template<typename T> bool get_value(const std::vector<std::string>& optionsList, const std::string& key, T& val)
{
	if (optionsList.empty())
		return false;

	bool found = false;

	for (unsigned i = 0; i<optionsList.size(); i++) {

		const std::string& opt = optionsList.at(i);
		int begin = opt.find_first_of("=", 0);
		int end = opt.length();

		if (!begin && (begin >= end))
			continue;

		std::string value = opt.substr(++begin, end);
		if (value.empty())
			continue;

		if (strstr(opt.c_str(), key.c_str())) {
			val = fromString<T>(value);
			found = true;
			break;
		}
	}

	return found;
}

//-----------------------------------------------------------------------------

template<typename T>
T get_from_cmdline(int argc, char **argv, const char* name, T defValue)
{
    T res(defValue);
    for( int ii=1; ii<argc-1; ii++ ) {
        if(strcmp(argv[ii], name) == 0) {
            std::string val = argv[ii+1];
            res = fromString<T>(val);
        }
    }
    return res;
}

//------------------------------------------------------------------------------

template<typename T>
bool get_from_cmdline(int argc, char **argv, const char* name, T defValue, T& value)
{
    value = defValue;
    for( int ii=1; ii<argc-1; ii++ ) {
        if(strcmp(argv[ii], name) == 0) {
            std::string val = argv[ii+1];
            value = fromString<T>(val);
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------

template<typename T>
bool parse_line(const std::string &str, std::vector<T> &data)
{
    data.clear();

    // заменяем символы табуляции в строке на пробелы
    //for(unsigned i=0; i<str.length(); i++) {
    //    if(str.at(i) == '\t' || str.at(i) == '\n')
    //        str[i] = ' ';
    //}

    int start = 0;
    int stop = 0;

    do {

        start = str.find_first_not_of(" ", stop);
        stop = str.find_first_of(" ", start);

        if(start == -1)
            break;

        if(stop == -1) {
            stop = str.length();
        }

        std::string s = str.substr(start,stop - start);

        if(!check_string_format(s))
            continue;

        T value = fromString<T>(s);
        data.push_back(value);

    } while (1);

    if(data.empty())
        return false;
    return true;
}

//-----------------------------------------------------------------------------
template<typename T>
bool get_filter_data(const std::string& fname, std::vector<T>& data)
{
    std::fstream ifs;
    ifs.open(fname.c_str(), std::ios::in);
    if (!ifs.is_open()) {
        return false;
    }

	int sym = 0;
    while(ifs >> sym) {
		data.push_back(T(sym));
    }

    return !data.empty();    
}

//-----------------------------------------------------------------------------

#endif // CONFIG_PARSER_H
