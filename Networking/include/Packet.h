#pragma once
#include <string>
#include <vector>

namespace Networking
{
	class Packet
	{
	public:

		Packet();
		Packet(const Packet& other) = default;
		Packet& operator=(const Packet& other) = default;
		~Packet();

		void Append(const void* data, std::size_t sizeInBytes);
		void Clear();

		const void* GetData() const;
		const void* GetData(std::size_t offset) const;

		std::size_t GetDataSize() const;

		bool EndOfPacket() const;

		//Read
		Packet& operator >> (unsigned char& data);
		Packet& operator >> (unsigned int& data);
		Packet& operator >> (std::string& data);
		//Write
		Packet& operator << (unsigned int& data);
		Packet& operator << (unsigned char& data);
		Packet& operator << (const char* data);
		Packet& operator << (const std::string& data);

		bool CheckSize(std::size_t size);
	private:
		std::vector<char> m_data;
		std::size_t m_readPosition;
		std::size_t m_sendPos;
		bool m_isValid;
	};
}
