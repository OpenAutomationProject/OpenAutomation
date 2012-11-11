/*
 * Author:  Jax Roland
 * Date:    09.11.2012
 * License: GPLv3
 */

#include <stdio.h>
#include <string.h>

#include "vaillant.h"

const char *pcDayOfWeek[] = {"","Mon","Tue","Wed","Thu","Fri","Sat","Sun"};

// valid QQ bytes
const unsigned char m_arrucValidQQ[] = {'\x00','\x01','\x10'};
const int iQQSize = sizeof(m_arrucValidQQ) / sizeof(unsigned char);

// valid ZZ bytes
const unsigned char m_arrucValidZZ[] = {'\x08','\x0a','\x23','\x25','\x50','\x52','\x53','\xec','\xed','\xfc','\xfe'};
const int iZZSize = sizeof(m_arrucValidZZ) / sizeof(unsigned char);

// valid PB bytes
const unsigned char m_arrucValidPB[] = {'\x01','\x07','\xb5','\xff'};
const int iPBSize = sizeof(m_arrucValidPB) / sizeof(unsigned char);

// arrays / vaillant command

// Aussentemperatur
const unsigned char m_arruc_10_fe_b5_16_03_01[] = {'\x10','\xfe','\xb5','\x16','\x03','\x01'};
//
const unsigned char m_arruc_10_fe_b5_16_03_04[] = {'\x10','\xfe','\xb5','\x16','\x03','\x04'};
//Datum / Uhrzeit
const unsigned char m_arruc_10_fe_b5_16_08_00[] = {'\x10','\xfe','\xb5','\x16','\x08','\x00'};
//
const unsigned char m_arruc_10_fe_b5_05_03_a4[] = {'\x10','\xfe','\xb5','\x05','\x03','\xa4'};
// Vorlauftemperatur
const unsigned char m_arruc_10_fe_b5_05_04_27[] = {'\x10','\xfe','\xb5','\x05','\x04','\x27'};
// Außentemperatur + Datum / Uhrzeit
const unsigned char m_arruc_10_fe_07_00_09[]    = {'\x10','\xfe','\x07','\x00','\x09'};
// ??? - Vorgabe ? fast jede Sekunde
const unsigned char m_arruc_10_08_b5_09_03_29[] = {'\x10','\x08','\xb5','\x09','\x03','\x29'};

//
const unsigned char m_arruc_10_08_b5_10_09_00[] = {'\x10','\x08','\xb5','\x10','\x09','\x00'};

//
const unsigned char m_arruc_10_08_b5_13_03_04[] = {'\x10','\x08','\xb5','\x13','\x03','\x04'};

//
const unsigned char m_arruc_10_ed_b5_08_07_07[] = {'\x10','\xed','\xb5','\x08','\x07','\x07'};


// Mischerkreis 0x50
const unsigned char m_arruc_10_50_b5_04_01_00[] = {'\x10','\x50','\xb5','\x04','\x01','\x00'};
const unsigned char m_arruc_10_50_b5_04_01_31[] = {'\x10','\x50','\xb5','\x04','\x01','\x31'};
const unsigned char m_arruc_10_50_b5_04_01_32[] = {'\x10','\x50','\xb5','\x04','\x01','\x32'};
const unsigned char m_arruc_10_50_b5_04_01_33[] = {'\x10','\x50','\xb5','\x04','\x01','\x33'};

// 0x50 0x52 0x53 Vorgabe Heizkreis (Betriebsart, Raumsolltemperatur, Absenktemperatur)
const unsigned char m_arruc_10_50_b5_09_04_0e[] = {'\x10','\x50','\xb5','\x09','\x04','\x0e'};

// 0x25 Vorgabe Warmwasser (Betriebsart)
const unsigned char m_arruc_10_25_b5_09_04_0e[] = {'\x10','\x25','\xb5','\x09','\x04','\x0e'};

// 0x25 Vorgabe Warmwasser (min. Warmwassertemperatur)
const unsigned char m_arruc_10_25_b5_09_05_0e[] = {'\x10','\x25','\xb5','\x09','\x05','\x0e'};

void print_ebus_msg_head(struct MSG_HEAD_t *pstMsgHead) {
//	fprintf(stdout, "\nQQ ZZ  PB SB  NN\n");
	fprintf(stdout, "\n%02x %02x  %02x %02x  %02x %02x ", pstMsgHead->QQ, pstMsgHead->ZZ, pstMsgHead->PB, pstMsgHead->SB, pstMsgHead->NN, pstMsgHead->TB);
}

// find ucByte from given ucByte Array
int is_valid(unsigned char ucByte, const unsigned char *pucData, int iDataSize) {
	int iCount;
	for (iCount = 0; iCount < iDataSize; iCount++)
		if (pucData[iCount] == ucByte)
			return 1;
	return 0;
}

void print_ebus_msg(unsigned char *pucEbusMsg, int iEbusMsgSize) {

	int iReturn;
	int iResult;
	float fResult;

	// 10_fe_b5_16_03_01 - Aussentemperatur
	if ( memcmp((const char *) &m_arruc_10_fe_b5_16_03_01[0],  pucEbusMsg, sizeof(m_arruc_10_fe_b5_16_03_01)) == 0 ) {
//		if (data2b_to_int(pucEbusMsg[7],  pucEbusMsg[6], &result_int, &result_dec_centi, NULL) !=0 ) {
		iReturn = data2b_to_float(pucEbusMsg[6], pucEbusMsg[7], &fResult);

		fprintf(stdout, " - Außentemperatur: %8.3f°C", fResult);

	}

	// 10_fe_b5_16_03_04 - ???
//	if ( memcmp((const char *) &m_arruc_10_fe_b5_16_03_04[0],  pucEbusMsg, sizeof(m_arruc_10_fe_b5_16_03_04)) == 0 ) {
//
//		if (data2b_to_int(pucEbusMsg[7],  pucEbusMsg[6], &result_int, &result_dec_centi, NULL) !=0 ) {
//			if (result_dec_centi % 10 == 0)
//				fprintf(stdout, " - temp?: -%d.%01d°C", result_int, ((int)result_dec_centi)/10);
//			else
//				fprintf(stdout, " - temp?: %d.%02d°C", result_int, (int)result_dec_centi);
//		}
//
//	}

	// 10_fe_b5_16_08_00 - Datum / Uhrzeit
	if ( memcmp((const char *) &m_arruc_10_fe_b5_16_08_00[0],  pucEbusMsg, sizeof(m_arruc_10_fe_b5_16_08_00)) == 0 ) {

		bcd_to_int(pucEbusMsg[11], &iResult);
		fprintf(stdout, " - %s, ", pcDayOfWeek[iResult]);

		bcd_to_int(pucEbusMsg[9], &iResult);
		printf("%02d.", iResult);
		bcd_to_int(pucEbusMsg[10], &iResult);
		printf("%02d.", iResult);
		bcd_to_int(pucEbusMsg[12], &iResult);
		printf("%02d ", iResult);

		bcd_to_int(pucEbusMsg[8], &iResult);
		printf("%02u:", iResult);
		bcd_to_int(pucEbusMsg[7], &iResult);
		printf("%02u:", iResult);
		bcd_to_int(pucEbusMsg[6], &iResult);
		printf("%02u", iResult);
	}

	// 10_fe_b5_04_01_27 - Vorlauftemperatur
	if ( memcmp((const char *) &m_arruc_10_fe_b5_05_04_27[0],  pucEbusMsg, sizeof(m_arruc_10_fe_b5_05_04_27)) == 0 ) {

		if (pucEbusMsg[6] == 0x01)
			fprintf(stdout, " - Speicherladung aktiv");

		iReturn = data1c_to_float(pucEbusMsg[7], &fResult);

		printf(" - Vorlauftemperatur: %5.1f°C", fResult);

	}

	// 10_fe_07_00_09 - Außentemperatur + Datum / Uhrzeit
	if ( memcmp((const char *) &m_arruc_10_fe_07_00_09[0],  pucEbusMsg, sizeof(m_arruc_10_fe_07_00_09)) == 0 ) {

		//if (data2b_to_int(pucEbusMsg[6],  pucEbusMsg[5], &result_int, &result_dec_centi, NULL) !=0 ) {
		iReturn = data2b_to_float(pucEbusMsg[5], pucEbusMsg[6], &fResult);

		printf(" - Außentemperatur: %8.3f°C ", fResult);


		bcd_to_int(pucEbusMsg[12], &iResult);
		printf("%s, ", pcDayOfWeek[iResult + 1]);

		bcd_to_int(pucEbusMsg[10], &iResult);
		printf("%02d.", iResult);
		bcd_to_int(pucEbusMsg[11], &iResult);
		printf("%02d.", iResult);
		bcd_to_int(pucEbusMsg[13], &iResult);
		printf("%02d ", iResult);

		bcd_to_int(pucEbusMsg[9], &iResult);
		printf("%02u:", iResult);
		bcd_to_int(pucEbusMsg[8], &iResult);
		printf("%02u:", iResult);
		bcd_to_int(pucEbusMsg[7], &iResult);
		printf("%02u", iResult);

	}

	// 10_08_b5_10_09_00
	if ( memcmp((const char *) &m_arruc_10_08_b5_10_09_00[0],  pucEbusMsg, sizeof(m_arruc_10_08_b5_10_09_00)) == 0 ) {

		if (pucEbusMsg[6] == 0x02)
			fprintf(stdout, " - Wärmepumpe Ein (%02x)", pucEbusMsg[6]);
		else
			fprintf(stdout, " - Wärmepumpe Aus (%02x)", pucEbusMsg[6]);

		iReturn = data1c_to_float(pucEbusMsg[7], &fResult);

		printf(" - Soll Vorlauftemp: %5.1f°C", fResult);


	}

	// 10_ed_b5_08_07_07
	if ( memcmp((const char *) &m_arruc_10_ed_b5_08_07_07[0],  pucEbusMsg, sizeof(m_arruc_10_ed_b5_08_07_07)) == 0 ) {

//		signed int result_int;
//		unsigned char result_dec_centi;
//		int iReturn;
//
//		iReturn = data1c_to_int(pucEbusMsg[11], &result_int);
//		if (iReturn == -1)
//			fprintf(stdout, " - [11]: -1 : %d", result_int);
//		if (iReturn == 1)
//			fprintf(stdout, " - [11]:  1 : %d", result_int);
//

//		if (data2b_to_int(pucEbusMsg[9],  pucEbusMsg[8], &result_int, &result_dec_centi, NULL) !=0 ) {
//			if (result_dec_centi % 10 == 0)
//				printf(" - 10_ed_b5_08_07_07: -%d.%01d°C - ", result_int, ((int)result_dec_centi)/10);
//			else
//				printf(" - 10_ed_b5_08_07_07: %d.%02d°C - ", result_int, (int)result_dec_centi);
//		}
//
//		if (data2b_to_int(pucEbusMsg[11],  pucEbusMsg[10], &result_int, &result_dec_centi, NULL) !=0 ) {
//			if (result_dec_centi % 10 == 0)
//				printf(" - 10_ed_b5_08_07_07: -%d.%01d°C - ", result_int, ((int)result_dec_centi)/10);
//			else
//				printf(" - 10_ed_b5_08_07_07: %d.%02d°C - ", result_int, (int)result_dec_centi);
//		}

//		if (pucEbusMsg[6] == 0x02)
//			printf(" - Wärmepumpe Ein (%02x)", pucEbusMsg[6]);
//		else
//			printf(" - Wärmepumpe Aus (%02x)", pucEbusMsg[6]);

//		iReturn = data1c_to_int(pucEbusMsg[7], &result_int);
//		if (iReturn == -1)
//			printf(" - Soll Vorlauftemp: -1 : %d", result_int);
//		if (iReturn == 1)
//			printf(" - Soll Vorlauftemp:  1 : %d", result_int);


	}
/*
	// 10_08_b5_13_03_04
	if ( memcmp((const char *) &m_arruc_10_08_b5_13_03_04[0],  pucEbusMsg, sizeof(m_arruc_10_08_b5_13_03_04)) == 0 ) {


		if (data2b_to_int(pucEbusMsg[5],  pucEbusMsg[6], &result_int, &result_dec_centi, NULL) !=0 ) {
			if (result_dec_centi % 10 == 0)
				fprintf(stdout, " - ???: -%d.%01d", result_int, ((int)result_dec_centi)/10);
			else
				fprintf(stdout, " - ???: %d.%02d", result_int, (int)result_dec_centi);
		}

		iReturn = data1c_to_int(pucEbusMsg[6], &result_int);
		if (iReturn == -1)
			fprintf(stdout, " - ??: -1 : %d", result_int);
		if (iReturn == 1)
			fprintf(stdout, " - ??:  1 : %d", result_int);

		iReturn = data1c_to_int(pucEbusMsg[7], &result_int);
		if (iReturn == -1)
			fprintf(stdout, " - ??: -1 : %d", result_int);
		if (iReturn == 1)
			fprintf(stdout, " - ??:  1 : %d", result_int);

		ucBCD = bcd_to_u8(pucEbusMsg[6]);
		fprintf(stdout, " %02d", ucBCD);

		ucBCD = bcd_to_u8(pucEbusMsg[7]);
		fprintf(stdout, " %02d", ucBCD);


	}

	// 10_08_b5_09_03_29
	if ( memcmp((const char *) &m_arruc_10_08_b5_09_03_29[0],  pucEbusMsg, sizeof(m_arruc_10_08_b5_09_03_29)) == 0 ) {


		iReturn = data1c_to_int(pucEbusMsg[6], &result_int);
		if (iReturn == -1)
			fprintf(stdout, " - ?? -1 : %d", result_int);
		if (iReturn == 1)
			fprintf(stdout, " - ??  1 : %d", result_int);

		iReturn = data1c_to_int(pucEbusMsg[7], &result_int);
		if (iReturn == -1)
			fprintf(stdout, " - ? -1 : %d", result_int);
		if (iReturn == 1)
			fprintf(stdout, " - ?  1 : %d", result_int);


		if (data2b_to_int(pucEbusMsg[7],  pucEbusMsg[6], &result_int, &result_dec_centi, NULL)!=0) {
			if (result_dec_centi % 10 == 0)
				fprintf(stdout, " - : %d.%01d", result_int, ((int)result_dec_centi)/10);
			else
				fprintf(stdout, " - : %d.%02d", result_int, (int)result_dec_centi);
		}

		ucBCD = bcd_to_u8(pucEbusMsg[6]);
		fprintf(stdout, " %02d", ucBCD);

		ucBCD = bcd_to_u8(pucEbusMsg[7]);
		fprintf(stdout, " %02d", ucBCD);

	}
*/

	// 10_50_b5_09_04_0e - Vorgabe Heizkreis
	if ( memcmp((const char *) &m_arruc_10_50_b5_09_04_0e[0],  pucEbusMsg, sizeof(m_arruc_10_50_b5_09_04_0e)) == 0 ) {

		// 0x2b - Betriebsart
		if (pucEbusMsg[6] == 0x2b) {
			if (pucEbusMsg[8] == 0x01)
				fprintf(stdout, " - Heizkreis - Betriebsart: 01 - Heizen");
			else if (pucEbusMsg[8] == 0x02)
				fprintf(stdout, " - Heizkreis - Betriebsart: 02 - Aus");
			else if (pucEbusMsg[8] == 0x03)
				fprintf(stdout, " - Heizkreis - Betriebsart: 03 - Auto");
			else if (pucEbusMsg[8] == 0x04)
				fprintf(stdout, " - Heizkreis - Betriebsart: 04 - Eco");
			else if (pucEbusMsg[8] == 0x05)
				fprintf(stdout, " - Heizkreis - Betriebsart: 05 - Absenken");
			else
				fprintf(stdout, " - Heizkreis - Betriebsart: ??? %02x", pucEbusMsg[8]);
		}

		// 0x32 - Raumsolltemperatur
		if (pucEbusMsg[6] == 0x32) {
			iReturn = data1c_to_float(pucEbusMsg[8], &fResult);
			fprintf(stdout, " - Heizkreis - Raumsolltemperatur: %5.1f°C", fResult);
		}

		// 0x33 - Absenktemperatur
		if (pucEbusMsg[6] == 0x33) {
			iReturn = data1c_to_float(pucEbusMsg[8], &fResult);
				fprintf(stdout, " - Heizkreis - Absenktemperatur: %5.1f°C", fResult);
		}


		if (pucEbusMsg[6] != 0x2b && pucEbusMsg[6] != 0x32 && pucEbusMsg[6] != 0x33) {
			fprintf(stdout, " - Heizkreis:  %02x %02x %02x", pucEbusMsg[6], pucEbusMsg[7], pucEbusMsg[8]);
		}

	}

	// 10_25_b5_09_04_0e - Vorgabe Warmwasser - Betriebsart
	if ( memcmp((const char *) &m_arruc_10_25_b5_09_04_0e[0],  pucEbusMsg, sizeof(m_arruc_10_25_b5_09_04_0e)) == 0 ) {

		// 0x2b - Betriebsart
		if (pucEbusMsg[6] == 0x2b) {
			if (pucEbusMsg[8] == 0x01)
				fprintf(stdout, " - Warmwasser - Betriebsart: 01 - Ein");
			else if (pucEbusMsg[8] == 0x02)
				fprintf(stdout, " - Warmwasser - Betriebsart: 02 - Aus");
			else if (pucEbusMsg[8] == 0x03)
				fprintf(stdout, " - Warmwasser - Betriebsart: 03 - Auto");
			else
				fprintf(stdout, " - Warmwasser - Betriebsart: ??? %02x", pucEbusMsg[8]);
		}

		if (pucEbusMsg[6] != 0x2b) {
			fprintf(stdout, " - Warmwasser:  %02x %02x %02x", pucEbusMsg[6], pucEbusMsg[7], pucEbusMsg[8]);
		}

	}

	// 10_25_b5_09_05_0e - Vorgabe Warmwasser - min. Warmwassertemperatur
	if ( memcmp((const char *) &m_arruc_10_25_b5_09_04_0e[0],  pucEbusMsg, sizeof(m_arruc_10_25_b5_09_04_0e)) == 0 ) {

		// 0x83 - min. Warmwassertemperatur
		if (pucEbusMsg[6] == 0x83) {

			//cals data2c_to_float


			if (pucEbusMsg[8] == 0x01)
				fprintf(stdout, " - Warmwasser - min. Warmwassertemperatur: 01 - Ein");
			else if (pucEbusMsg[8] == 0x02)
				fprintf(stdout, " - Warmwasser - min. Warmwassertemperatur: 02 - Aus");
			else if (pucEbusMsg[8] == 0x03)
				fprintf(stdout, " - Warmwasser - min. Warmwassertemperatur: 03 - Auto");
			else
				fprintf(stdout, " - Warmwasser - min. Warmwassertemperatur: ??? %02x", pucEbusMsg[8]);
		}

		if (pucEbusMsg[6] != 0x83) {
			fprintf(stdout, " - Warmwasser:  %02x %02x %02x %02x", pucEbusMsg[6], pucEbusMsg[7], pucEbusMsg[8], pucEbusMsg[8]);
		}

	}

}

// decode ebus messages
void decode_ebus_msg(unsigned char *pucDataBuffer, int iDataBufferSize) {

//	int jj;
	int iCount = 0;
	int iPrint;
	int iMsgLen;
	int iMsgLenMaster;
	int iMsgLenSlave;
	int iCRCCalcLen;
	int iCRCMsgPos;
	int iACKMsgPos;
	unsigned char ucCRC;
//	unsigned char arrucTmpBuffer[BUFFER_SIZE];

//	struct MSG_HEAD_t *pstMsgHead;

//	pstMsgHead = (MSG_HEAD_t *) pucDataBuffer;


	//print_ebus_msg_head(pstMsgHead);

	// reset tmp buffer
//	memset(arrucTmpBuffer, '\0', sizeof(arrucTmpBuffer));


	// print raw data
	if (g_iVerbose) {
		fprintf(stdout, "\nraw:  ");
		for (iCount = 0; iCount < iDataBufferSize; iCount++)
			fprintf(stdout, " %02x", pucDataBuffer[iCount]);

		fprintf(stdout, "\n");
		iCount = 0;
	}

	while (iCount < iDataBufferSize) {
		// search for valid PB Byte
		if(is_valid(pucDataBuffer[iCount], m_arrucValidPB, iPBSize))
			if (iCount >= 2) {
				// are QQ and ZZ bytes valid ?
				if (is_valid(pucDataBuffer[iCount - 2], m_arrucValidQQ, iQQSize) &&
					is_valid(pucDataBuffer[iCount - 1], m_arrucValidZZ, iZZSize) ) {

					// TODO parse new message for 0xAA or 0xa9 + 0x01

					// Master - Broadcast with CRC
					if (pucDataBuffer[iCount - 1] == EBUS_MSG_BROADCAST) {
						// 5(QQ+ZZ+PB+SB+NN) + Len(NN) + 1(CRC)
						iMsgLenMaster = 5 + pucDataBuffer[iCount + 2] + 1;

						// calc CRC for message
						iCRCCalcLen = iMsgLenMaster - 1;
						ucCRC = calc_crc(&pucDataBuffer[iCount - 2], iCRCCalcLen);

						iCRCMsgPos = iCount + iMsgLenMaster - 3;

						// msg CRC == calc CRC
						if (ucCRC == pucDataBuffer[iCRCMsgPos])
							fprintf(stdout, "C");
						else
							fprintf(stdout, "-");

						// - for ACK; L for LEN; B for Broadcast
						fprintf(stdout, "BL---");

						iMsgLen = iMsgLenMaster;
					}

					// Exotic - '00 0a ff ff ff ff ff ff ff ff ff ff 0f'
					else if (pucDataBuffer[iCount - 1] == VAILLANT_ZZ_0A) {
						// Len('00 0a ff ff ff ff ff ff ff ff ff ff 0f')
						iMsgLenMaster = 13;

						fprintf(stdout, "UNKOWN");

						iMsgLen = iMsgLenMaster;
					}

					// TODO: Master - Slave with CRC ACK (S)NN (S)DB (S)CRC ACK
					// Master 0x10 - Slave 0xed (Solar)
					else if (pucDataBuffer[iCount - 1] == VAILLANT_ZZ_ED) {
						// Master
						// 5(QQ+ZZ+PB+SB+NN) + Len(NN) + 2(CRC+ACK)
						iMsgLenMaster = 5 + pucDataBuffer[iCount + 2] + 2;

						// calc CRC for message
						iCRCCalcLen = iMsgLenMaster - 2;
						ucCRC = calc_crc(&pucDataBuffer[iCount - 2], iCRCCalcLen);

						iCRCMsgPos = iCount + iMsgLenMaster - 4;

						// msg CRC == calc CRC
						if (ucCRC == pucDataBuffer[iCRCMsgPos])
							fprintf(stdout, "C");
						else
							fprintf(stdout, "-");

						// check ACK
						iACKMsgPos = iCount + iMsgLenMaster - 3;
						if (EBUS_MSG_ACK == pucDataBuffer[iACKMsgPos])
							fprintf(stdout, "A");
						else
							fprintf(stdout, "-");

						// L for LEN; - for no Broadcast
						fprintf(stdout, "L");

						// Slave
						// Len(NN) + 2(CRC+ACK)
						iMsgLenSlave = pucDataBuffer[iCount + iMsgLenMaster - 2] + 2;

						// calc CRC for message
						iCRCCalcLen = iMsgLenSlave - 1;
						ucCRC = calc_crc(&pucDataBuffer[iCount + iMsgLenMaster - 2], iCRCCalcLen);

						iCRCMsgPos = iCount + iMsgLenMaster - 2 + iMsgLenSlave - 1;

						// msg CRC == calc CRC
						if (ucCRC == pucDataBuffer[iCRCMsgPos])
							fprintf(stdout, "C");
						else
							fprintf(stdout, "-");

						// check ACK
						iACKMsgPos = iCount + iMsgLenMaster - 2 + iMsgLenSlave;
						if (EBUS_MSG_ACK == pucDataBuffer[iACKMsgPos])
							fprintf(stdout, "A");
						else
							fprintf(stdout, "-");

						// L for LEN; - for no Broadcast
						fprintf(stdout, "L");

						iMsgLen = iMsgLenMaster + iMsgLenSlave + 1;
						iCRCMsgPos = iCount + iMsgLenMaster - 4;
					}

					// Master - Master with CRC and ACK
					else {
						// 5(QQ+ZZ+PB+SB+NN) + Len(NN) + 2(CRC+ACK)
						iMsgLenMaster = 5 + pucDataBuffer[iCount + 2] + 2;

						// calc CRC for message
						iCRCCalcLen = iMsgLenMaster - 2;
						ucCRC = calc_crc(&pucDataBuffer[iCount - 2], iCRCCalcLen);

						iCRCMsgPos = iCount + iMsgLenMaster - 4;

						// msg CRC == calc CRC
						if (ucCRC == pucDataBuffer[iCRCMsgPos])
							fprintf(stdout, "C");
						else
							fprintf(stdout, "-");

						// check ACK
						iACKMsgPos = iCount + iMsgLenMaster - 3;
						if (EBUS_MSG_ACK == pucDataBuffer[iACKMsgPos])
							fprintf(stdout, "A");
						else
							fprintf(stdout, "-");

						// L for LEN; - for no Broadcast
						fprintf(stdout, "L---");

						iMsgLen = iMsgLenMaster;

					}

					// print ebus message - hexdump
					for (iPrint = iCount - 2; iPrint < ((iCount - 2) + iMsgLen); iPrint++)
							fprintf(stdout, " %02x", pucDataBuffer[iPrint]);

					// print ebus message - decoded
					if (g_iFlagDecodeMsg)
						print_ebus_msg(&pucDataBuffer[iCount - 2], 0);

					fprintf(stdout, "\n");

					fflush(stdout);

				}

			}

		iCount++;
	}

}
