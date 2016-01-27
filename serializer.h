/**@file
 *
 * @date Jan 26, 2016
 * @author Craig Hesling <craig@hesling.com>
 */

#ifndef SERIALIZER_SERIALIZER_H_
#define SERIALIZER_SERIALIZER_H_

/**
 * @brief Specifies which method was used to serialize the data.
 */
typedef enum serial_type {
	SERIAL_TYPE_BARE,
	SERIAL_TYPE_WITHTABLE
} serial_type_t;

typedef struct serial_data {
	serial_type_t type;

	/**
	 * The offset from payload, where the first data item can be found.
	 */
	size_t data_items_off;

	/**
	 * The total size of the remaining payload
	 */
	size_t payload_size;

	/**
	 * This is the starting point for all offsets
	 */
	char payload[];
} *serial_data_t;



typedef enum data_type {
	DATA_TYPE_CHAR     = 'c',
	DATA_TYPE_SHORT    = 'h',
	DATA_TYPE_INT      = 'd',
	DATA_TYPE_LONG     = 'l',
	DATA_TYPE_LONGLONG = 'i',
//	DATA_TYPE_STRING   = 'S',
	DATA_TYPE_BUFFER   = 'B'
} data_type_t;


serial_data_t serial_pack_extra(serial_type_t type, const char *fmt, ...);
serial_data_t serial_pack(const char *fmt, ...);
int getnext_int(serial_data_t sdata);

size_t serial_item_count(serial_data_t sdata);

#endif /* SERIALIZER_SERIALIZER_H_ */
