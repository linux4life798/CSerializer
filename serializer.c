/**@file
 *
 * This serializer favors compression over calculation.
 *
 * @date Jan 26, 2016
 * @author Craig Hesling <craig@hesling.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h> // NULL
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
//#include <error.h>
#include <assert.h>
#include "serializer.h"

#define OFF_PTRDIFF(ptr_hi, ptr_lo) (((size_t)(ptr_hi))-((size_t)(ptr_lo)))
/** Unsigned offset */
#define PTR_UOFFSET(ptr, off) ( ((char *)(ptr)) + ((size_t)(off)) )
/** Signed offset */
#define PTR_SOFFSET(ptr, off) ( ((char *)(ptr)) + ((ssize_t)(off)) )

/*-- Table Data Types --*/

/**
 * @brief Information about a packed data item
 */
struct item_info {
	enum data_type type;
	size_t payload_off;
};

/**
 * @brief A table that contains information about packed data items
 */
struct item_info_table {
	size_t count;            ///< Number of info items
	struct item_info info[]; ///< The info items
};

/*-- Data Item Type --*/

/**
 * @brief A data item
 */
struct item {
	enum data_type type : 8;
	union {
		/** reference to storage for primitive data types */
		union {
			unsigned char      CHAR;
			unsigned short     SHORT;
			unsigned int       INT;
			unsigned long      LONG;
			unsigned long long LONGLONG;
		} prim;

		/** reference to storage for array data types */
		struct {
			size_t buf_size;
			char buf[];
		} array;
	} data;
};


static inline
size_t getprimsize(data_type_t type) {
	switch(type) {
	case DATA_TYPE_CHAR: return sizeof(char);
	case DATA_TYPE_SHORT: return sizeof(short);
	case DATA_TYPE_INT: return sizeof(int);
	case DATA_TYPE_LONG: return sizeof(long);
	case DATA_TYPE_LONGLONG: return sizeof(long long);
	default: assert(0);
	}
}

#define CASE_PRIM_SIZESUM(ch, type, var) case ch: (var)+=sizeof(type); break;

serial_data_t serial_pack_vextra(serial_type_t type, const char *fmt, va_list va) {
	va_list ap;
	serial_data_t sdata;
	const char *fmt_ch;
	size_t item_count = 0;
	size_t item_index = 0;
	size_t table_total_size = 0;
	size_t payload_total_size = 0;
	struct item_info_table *table;
	struct item *item;

	/*TODO Implement table serial model */
	assert(type == SERIAL_TYPE_BARE);
	assert(type==SERIAL_TYPE_BARE || type==SERIAL_TYPE_HASTABLE);

	/* count number of items and their sizes */

	item_count = 0;
	va_copy(ap, va);
	fmt_ch = fmt;
	while(*fmt_ch++) {
		item_count++;
		if(isupper(*fmt_ch)) {
			payload_total_size += va_arg(ap, size_t);
			va_arg(ap, void *);
		} else {
			payload_total_size += getprimsize(*fmt_ch);
			// jog the va list to stay in line for arrays
			switch(*fmt_ch) {
			case DATA_TYPE_CHAR:     va_arg(ap, char); break;
			case DATA_TYPE_SHORT:    va_arg(ap, short); break;
			case DATA_TYPE_INT:      va_arg(ap, int); break;
			case DATA_TYPE_LONG:     va_arg(ap, long); break;
			case DATA_TYPE_LONGLONG: va_arg(ap, long long); break;
			default: assert(0);
			}

		}
	}

	/* add space for a table */
	if(type == SERIAL_TYPE_HASTABLE) {
		table_total_size = sizeof(struct item_info_table);
		table_total_size += sizeof(struct item_info)*item_count;
		payload_total_size += table_total_size;
	}

	sdata = (serial_data_t)malloc(sizeof(struct serial_data)+payload_total_size);
	if(sdata == NULL) {
		perror("couldn't allocate a serial_data_t");
	}
	sdata->type = type;
	sdata->data_items_off = 0;
	sdata->payload_size = payload_total_size;

	/* fill in table */
	if(type == SERIAL_TYPE_HASTABLE) {
		table = (struct item_info_table *)sdata->payload;
		sdata->data_items_off = table_total_size;
		table->count = item_count;
	}

	/* copy in items and setup info table */

	item_index = 0;
	item = (struct item *)(sdata->payload+sdata->data_items_off);
	va_copy(ap, va);
	fmt_ch = fmt;
	while(*fmt_ch++) {
		size_t item_payload_off = OFF_PTRDIFF(item, sdata->payload);
		size_t item_size_total = sizeof(struct item);

		/* handle copying in an array */
		if(isupper(*fmt_ch)) {
			size_t buf_size = va_arg(ap, size_t);
			void  *buf      = va_arg(ap, void *);

			assert(buf_size > 0);
			assert(buf);

			if(type==SERIAL_TYPE_HASTABLE) {
				table->info[item_index].type = *fmt_ch;
				table->info[item_index].payload_off = item_payload_off;
			}
			item->type = *fmt_ch;
			item->data.array.buf_size = buf_size;
			memcpy(item->data.array.buf, buf, buf_size);

			item_size_total += buf_size;

		}
		/* handle copying in a primitive data type */
		else {
			switch(*fmt_ch) {
			case DATA_TYPE_CHAR:     item->data.prim.CHAR=va_arg(ap, char); break;
			case DATA_TYPE_SHORT:    item->data.prim.SHORT=va_arg(ap, short); break;
			case DATA_TYPE_INT:      item->data.prim.INT=va_arg(ap, int); break;
			case DATA_TYPE_LONG:     item->data.prim.LONG=va_arg(ap, long); break;
			case DATA_TYPE_LONGLONG: item->data.prim.LONGLONG=va_arg(ap, long long); break;
			default: assert(0);
			}

		}

		/* iterate to next item */
		item = (struct item *)PTR_UOFFSET(item, item_size_total);

		item_index++;
	}
}

serial_data_t serial_pack_extra(serial_type_t type, const char *fmt, ...) {
	va_list ap;
	serial_data_t sdata;
	va_start(fmt, ap);
	sdata = serial_pack_vextra(type, fmt, ap);
	va_end(ap);
	return sdata;
}

serial_data_t serial_pack(const char *fmt, ...) {
	va_list ap;
	serial_data_t sdata;
	va_start(fmt, ap);
	sdata = serial_pack_vextra(SERIAL_TYPE_BARE, fmt, ap);
	va_end(ap);
	return sdata;
}

/**
 * TODO: Implement
 */
void *getnext_buffer(serial_data_t data, void **buffer, size_t *buffer_size) {
	return *buffer;
}

/**
 * TODO: Implement
 */
int ser_getnext_int(serial_data_t data) {
	return 0;
}
