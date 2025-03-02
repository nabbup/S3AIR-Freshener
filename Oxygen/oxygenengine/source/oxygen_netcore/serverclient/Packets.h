/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen_netcore/network/LowLevelPackets.h"
#include "oxygen_netcore/network/VersionRange.h"
#include "oxygen_netcore/serverclient/ChannelBroadcastPackets.h"
#include "oxygen_netcore/serverclient/FileTransferPackets.h"


// Packets for communication between Oxygen server and client
namespace network
{

	// ----- Connectionless, low-level packets -----

	struct GetExternalAddressConnectionless : public lowlevel::PacketBase
	{
		uint8 mPacketVersion = 1;
		uint64 mQueryID = 0;

		static const constexpr uint16 SIGNATURE = 0x3bcf;
		virtual uint16 getSignature() const override  { return SIGNATURE; }

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serialize(mPacketVersion);
			serializer.serialize(mQueryID);
		}
	};

	struct ReplyExternalAddressConnectionless : public lowlevel::PacketBase
	{
		uint8 mPacketVersion = 1;
		uint64 mQueryID = 0;
		std::string mIP;
		uint16 mPort = 0;

		static const constexpr uint16 SIGNATURE = 0xf151;
		virtual uint16 getSignature() const override  { return SIGNATURE; }

		virtual void serializeContent(VectorBinarySerializer& serializer, uint8 protocolVersion) override
		{
			serializer.serialize(mPacketVersion);
			serializer.serialize(mQueryID);
			serializer.serialize(mIP, 64);
			serializer.serialize(mPort);
		}
	};



	// ----- High-level requests -----

	class GetServerFeaturesRequest : public highlevel::RequestBase
	{
		struct QueryData
		{
			// No parameters at all

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
			}
		};

		struct ResponseData
		{
			struct Feature
			{
				std::string mIdentifier;
				VersionRange<uint8> mVersions;

				inline Feature() {}
				inline Feature(const char* identifier, uint8 minVersion, uint8 maxVersion) : mIdentifier(identifier), mVersions(minVersion, maxVersion) {}
			};
			std::vector<Feature> mFeatures;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serializeArraySize(mFeatures, 0xff);
				for (Feature& feature : mFeatures)
				{
					serializer.serialize(feature.mIdentifier, 0xff);
					feature.mVersions.serialize(serializer);
				}
			}
		};

		HIGHLEVEL_REQUEST_DEFINE_FUNCTIONALITY("GetServerFeaturesRequest")
	};


	class GetExternalAddressRequest : public highlevel::RequestBase
	{
		struct QueryData
		{
			// No parameters at all

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
			}
		};

		struct ResponseData
		{
			std::string mIP;
			uint16 mPort = 0;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serialize(mIP, 64);
				serializer.serialize(mPort);
			}
		};

		HIGHLEVEL_REQUEST_DEFINE_FUNCTIONALITY("GetExternalAddressRequest")
	};


	class AppUpdateCheckRequest : public highlevel::RequestBase
	{
		struct QueryData
		{
			std::string mAppName;
			std::string mPlatform;
			std::string mReleaseChannel;
			uint32 mInstalledAppVersion = 0;
			uint32 mInstalledContentVersion = 0;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serialize(mAppName, 0xff);
				serializer.serialize(mPlatform, 0xff);
				serializer.serialize(mReleaseChannel, 0xff);
				serializer.serialize(mInstalledAppVersion);
				serializer.serialize(mInstalledContentVersion);
			}
		};

		struct ResponseData
		{
			bool mHasUpdate = false;
			uint32 mAvailableAppVersion = 0;
			uint32 mAvailableContentVersion = 0;
			std::string mUpdateInfoURL;

			inline void serializeData(VectorBinarySerializer& serializer, uint8 protocolVersion)
			{
				serializer.serialize(mHasUpdate);
				if (mHasUpdate)
				{
					serializer.serialize(mAvailableAppVersion);
					serializer.serialize(mAvailableContentVersion);
					serializer.serialize(mUpdateInfoURL, 0xff);
				}
			}
		};

		HIGHLEVEL_REQUEST_DEFINE_FUNCTIONALITY("AppUpdateCheck")
	};

}
