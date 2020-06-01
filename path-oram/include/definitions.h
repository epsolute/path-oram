#pragma once

#include <boost/format.hpp>
#include <climits>
#include <string>
#include <vector>

#ifndef INPUT_CHECKS
#define INPUT_CHECKS true
#endif

#ifndef USE_REDIS
#define USE_REDIS true
#endif

#ifndef USE_AEROSPIKE
#define USE_AEROSPIKE true
#endif

// use 256-bit security
#define KEYSIZE 32

// 256 bit hash size (SHA-256)
#define HASHSIZE 256
#define HASH_ALGORITHM EVP_sha256

// change to run all tests from different seed
#define TEST_SEED 0x13

namespace PathORAM
{
	using namespace std;

	// defines the integer type block ID
	// change (e.g. to unsigned int) if needed
	using number = unsigned long long;
	using uchar	 = unsigned char;
	using uint	 = unsigned int;
	using bytes	 = vector<uchar>;
	using block	 = pair<number, bytes>;
	using bucket = vector<block>;

	enum EncryptionMode
	{
		ENCRYPT,
		DECRYPT
	};

	enum BlockCipherMode
	{
		CBC,
		CTR,
		NONE
	};

	/**
	 * @brief Primitive exception class that passes along the excpetion message
	 *
	 * Can consume std::string, C-string and boost::format
	 */
	class Exception : public exception
	{
		public:
		explicit Exception(const char* message) :
			msg_(message)
		{
		}

		explicit Exception(const string& message) :
			msg_(message)
		{
		}

		explicit Exception(const boost::format& message) :
			msg_(boost::str(message))
		{
		}

		virtual ~Exception() throw() {}

		virtual const char* what() const throw()
		{
			return msg_.c_str();
		}

		protected:
		string msg_;
	};

	/**
	 * @brief global setting, block cipher mode which will be used for encryption
	 *
	 */
	inline BlockCipherMode __blockCipherMode = CBC;
}
