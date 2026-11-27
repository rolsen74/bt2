
/*
**          -+- Bluetooth -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "bt2_a2dp.h"

// --

int
BT2_ReadAudioBytes ( struct BTAudioSource * source, uint8_t * data, uint32_t length )
{
	int32_t bytes;

	while ( length )
	{
		if ( __atomic_load_n ( &source->Request->Cancel, __ATOMIC_RELAXED ) )
		{
			return ( 0 );
		}

		bytes = source->Read ( source->File, data, length );

		if ( ( bytes <= 0 ) || ( (uint32_t)bytes > length ) )
		{
			return ( 0 );
		}

		data += bytes;
		length -= bytes;
	}

	return ( 1 );
}

// --

static uint32_t
ReadLE32 ( const uint8_t * data )
{
	return ( BT_ReadLE16 ( data ) | ( (uint32_t)BT_ReadLE16 ( data + 2 ) << 16 ) );
}

// --

static int
ReadPCMExtension ( struct BTAudioSource * source, uint32_t bytes )
{
	static const uint8_t pcmGUID[] = { 1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71 };
	uint8_t				 extension[24];
	uint32_t			 index;
	uint16_t			 length;

	if ( ( bytes < sizeof ( extension ) ) || ( ! BT2_ReadAudioBytes ( source, extension, sizeof ( extension ) ) ) )
	{
		return ( 0 );
	}

	length = BT_ReadLE16 ( extension );

	if ( ( length < 22 ) || ( length > bytes - 2 ) || ( BT_ReadLE16 ( extension + 2 ) != 16 ) )
	{
		return ( 0 );
	}

	for ( index = 0; index < sizeof ( pcmGUID ); index++ )
	{
		if ( extension[8 + index] != pcmGUID[index] )
		{
			return ( 0 );
		}
	}

	return ( 1 );
}

// --

enum BTHCIResult
BT2_ReadWave ( struct BTAudioSource * source )
{
	uint8_t	 header[16];
	uint32_t remaining;
	uint32_t size;
	uint32_t skip;
	uint32_t rate;
	uint32_t chunks;
	uint16_t channels;
	uint16_t encoding;
	int		 format;

	format		 = 0;
	source->Live = 0;

	if ( ! BT2_ReadAudioBytes ( source, header, 12 ) )
	{
		return ( BT_A2DP_INVALID_WAVE );
	}

	if ( ( ReadLE32 ( header ) != 0x46464952 ) || ( ReadLE32 ( header + 8 ) != 0x45564157 ) )
	{
		return ( BT_A2DP_INVALID_WAVE );
	}

	remaining = ReadLE32 ( header + 4 );

	if ( remaining < 4 )
	{
		return ( BT_A2DP_INVALID_WAVE );
	}

	remaining -= 4;

	for ( chunks = 0; ( chunks < 256 ) && ( remaining >= 8 ); chunks++ )
	{
		if ( ! BT2_ReadAudioBytes ( source, header, 8 ) )
		{
			break;
		}

		remaining -= 8;

		size = ReadLE32 ( header + 4 );

		if ( ( size > remaining ) || ( ( size == remaining ) && ( size & 1 ) ) )
		{
			break;
		}

		skip = size + ( size & 1 );

		remaining -= skip;

		switch ( ReadLE32 ( header ) )
		{
			case 0x20746d66: // fmt
				if ( ( format ) || ( size < 16 ) || ( ! BT2_ReadAudioBytes ( source, header, 16 ) ) )
				{
					return ( BT_A2DP_INVALID_WAVE );
				}

				rate = ReadLE32 ( header + 4 );

				channels = BT_ReadLE16 ( header + 2 );

				if ( ( ( rate != 44100 ) && ( rate != 48000 ) ) || ( ( channels != 1 ) && ( channels != 2 ) )
					 || ( BT_ReadLE16 ( header + 14 ) != 16 ) || ( BT_ReadLE16 ( header + 12 ) != channels * 2 )
					 || ( ReadLE32 ( header + 8 ) != rate * channels * 2 ) )
				{
					return ( BT_A2DP_INVALID_WAVE );
				}

				encoding = BT_ReadLE16 ( header );

				if ( encoding == 0xfffe )
				{
					if ( ! ReadPCMExtension ( source, size - 16 ) )
					{
						return ( BT_A2DP_INVALID_WAVE );
					}

					skip -= 24;
				}
				else if ( encoding != 1 )
				{
					return ( BT_A2DP_INVALID_WAVE );
				}

				source->Channels	  = channels;
				source->Request->Rate = rate;
				skip -= 16;
				format = 1;

				break;

			case 0x61746164: // data
				if ( ( ! format ) || ( ! size ) || ( size % ( source->Channels * 2 ) ) )
				{
					return ( BT_A2DP_INVALID_WAVE );
				}

				source->BytesLeft = size;

				return ( BT_HCI_OK );
		}

		if ( ( skip ) && ( ! source->Skip ( source->File, skip ) ) )
		{
			break;
		}
	}

	return ( BT_A2DP_INVALID_WAVE );
}

// --
