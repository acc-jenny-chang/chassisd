/*************************************************************
 *       Copyright 2016 Accton Technology Corporation.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 ************************************************************/

#include "crc_util.h"


unsigned char crc_util_calculate_crc8_checksum(unsigned char crc, unsigned char data)
{
unsigned char i = 0;


	i = data ^ crc;
	crc = 0;
	if (i & 1)
		crc ^= 0x5e;
	if (i & 2)
		crc ^= 0xbc;
	if (i & 4)
		crc ^= 0x61;
	if (i & 8)
		crc ^= 0xc2;
	if (i & 0x10)
		crc ^= 0x9d;
	if (i & 0x20)
		crc ^= 0x23;
	if (i & 0x40)
		crc ^= 0x46;
	if (i & 0x80)
		crc ^= 0x8c;

	return crc;
}

unsigned char crc_util_calculate_crc8_checksum_of_buffer(unsigned char *data_buffer_ptr, unsigned long data_buffer_length)
{
unsigned char crc = 0;
unsigned long i = 0;


	for (i = 0, crc = 0; i < data_buffer_length; i++)
		crc = crc_util_calculate_crc8_checksum(crc, *(data_buffer_ptr + i));

	return crc;
}

