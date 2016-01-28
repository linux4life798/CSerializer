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

/**@struct serial_data
 * @brief The representation of serialized data
 *
 * Users should always use the \ref serial_data_t type,
 * which is a pointer of a struct managed by the serializer library.
 */
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

/**
 * @brief The state data for iterating over serial data's items
 */
typedef struct {
	/**@internal The reference to the serial data */
	serial_data_t sdata;
	/**@internal The current serial data item */
	struct item *item;
} serial_item_iterator_t;

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

/*-----------------------------------*
 *          Item Iterators           *
 *-----------------------------------*/

/* Sets up the iterator to point to the first item of the serial data */
serial_item_iterator_t *
serial_iit_begin(serial_item_iterator_t *it, serial_data_t sdata);
serial_item_iterator_t *
serial_iit_next(serial_item_iterator_t *it);
int serial_iit_hasnext(serial_item_iterator_t *it);
int serial_iit_isend(serial_item_iterator_t *it);

char   serial_iit_get_char   (serial_item_iterator_t *it);
short  serial_iit_get_short  (serial_item_iterator_t *it);
int    serial_iit_get_int    (serial_item_iterator_t *it);
const
void  *serial_iit_get_buf_ptr(serial_item_iterator_t *it);
size_t serial_iit_get_buf    (serial_item_iterator_t *it, void *buf);
size_t serial_iit_get_size   (serial_item_iterator_t *it);
data_type_t serial_iit_get_type(serial_item_iterator_t *it);

/*-----------------------------------*
 *            Utilities              *
 *-----------------------------------*/

/* Get the total size of serialized data - for copying the serialized data */
size_t serial_data_size    (serial_data_t sdata);

size_t serial_item_count   (serial_data_t sdata);
int    serial_item_get_int (serial_data_t sdata, size_t index);
void   serial_print_table  (serial_data_t sdata);
void   serial_print_items  (serial_data_t sdata);

#endif /* SERIALIZER_SERIALIZER_H_ */
