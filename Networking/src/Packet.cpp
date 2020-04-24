#include "Packet.h"
#include <WinSock2.h>
namespace Networking
{
	Packet::Packet() : m_readPosition(0), m_sendPos(0), m_isValid(true)
	{
	}

	Packet::~Packet()
	{
	}

	void Packet::Append(const void* data, std::size_t sizeInBytes)
	{
		if (data && (sizeInBytes > 0))
		{
			std::size_t start = m_data.size();
			m_data.resize(start + sizeInBytes);
			//std::copy_n(m_data[start], sizeInBytes, data);
			std::memcpy(&m_data[start], data, sizeInBytes);
		}
	}

	void Packet::Clear()
	{
		m_data.clear();
		m_readPosition = 0;
		m_isValid = true;
	}

	const void* Packet::GetData() const
	{
		return !m_data.empty() ? &m_data[0] : nullptr;
	}

	const void* Packet::GetData(std::size_t offset) const
	{
		return (!m_data.empty() && m_data.size() > offset) ? &m_data[offset] : nullptr;
	}

	std::size_t Packet::GetDataSize() const
	{
		return m_data.size();
	}

	bool Packet::EndOfPacket() const
	{
		return m_readPosition >= m_data.size();
	}

	bool Packet::CheckSize(std::size_t size)
	{
		m_isValid = m_isValid && (m_readPosition + size <= m_data.size());
		return m_isValid;
	}

	Packet& Packet::operator>>(unsigned char& data)
	{
		if (CheckSize(sizeof(data)))
		{
			data = *reinterpret_cast<const unsigned char*>(&m_data[m_readPosition]);
			m_readPosition += sizeof(data);
		}
		return *this;
	}

	Packet& Packet::operator>>(unsigned int& data)
	{
		if (CheckSize(sizeof(data)))
		{
			data = ntohl(*reinterpret_cast<const unsigned int*>(&m_data[m_readPosition]));
			m_readPosition += sizeof(data);
		}
		return *this;
	}

	Packet& Packet::operator>>(std::string& data)
	{
		unsigned int length = 0;
		*this >> length;
		data.clear();

		if ((length > 0) && CheckSize(length))
		{
			data.assign(&m_data[m_readPosition], length);


			m_readPosition += length;
		}
		return *this;
	}


	Packet& Packet::operator<<(unsigned int& data)
	{
		unsigned int write = htonl(data);
		Append(&write, sizeof(write));
		return *this;
	}

	Packet& Packet::operator<<(unsigned char& data)
	{
		Append(&data, sizeof(data));
		return *this;
	}

	Packet& Packet::operator<<(const char* data)
	{
		// Insert string length first
		auto length = static_cast<unsigned int>(std::strlen(data));
		*this << length;

		//Then insert chars
		Append(data, length * sizeof(char));
		return *this;
	}

	Packet& Packet::operator<<(const std::string& data)
	{
		// Insert string length first
		auto length = static_cast<unsigned int>(data.size());
		*this << length;

		//Then insert chars
		if (length > 0)
		{
			Append(data.c_str(), length * sizeof(std::string::value_type));
		}
		return *this;
	}


}
