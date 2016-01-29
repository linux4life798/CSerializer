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

#define IS_TYPE_ARRAY(type) isupper(type)

/*-- Table Data Types --*/

/**@internal
 * @brief Information about a packed data item
 */
struct item_info {
	enum data_type type;
	size_t payload_off;
};

/**@internal
 * @brief A table that contains information about packed data items
 */
struct item_info_table {
	size_t count;            ///< Number of info items
	struct item_info info[]; ///< The info items
};

/*-- Data Item Type --*/

/**@internal
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

static size_t
getitemsize(struct item *item) {
	assert(item);

	if (IS_TYPE_ARRAY(item->type)) {
		return item->data.array.buf_size;
	} else {
		switch (item->type) {
		case DATA_TYPE_CHAR:
			return sizeof(item->data.prim.CHAR);
		case DATA_TYPE_SHORT:
			return sizeof(item->data.prim.CHAR);
		case DATA_TYPE_INT:
			return sizeof(item->data.prim.CHAR);
		case DATA_TYPE_LONG:
			return sizeof(item->data.prim.CHAR);
		case DATA_TYPE_LONGLONG:
			return sizeof(item->data.prim.CHAR);
		default:
			assert(0);
		}
	}
}

/**@internal
 * @brief Return the entire size of an item in the payload.
 *
 * @note This is used when traversing items in the payload.
 *
 * @param item The item to investigate
 * @return The size of the item
 */
static size_t
getitemsizetotal(struct item *item) {
	assert(item);

	if(IS_TYPE_ARRAY(item->type)) {
		return sizeof(struct item)+item->data.array.buf_size;
	} else {
		return sizeof(struct item);
	}
}

/**@internal
 * @brief Traverse from one item to the next contiguous item.
 *
 * Fetches the next item using only the active elements information.
 * If the sdata parameter is supplied, the next item's location is
 * verified to be within the serial data payload. When sdata is provided,
 * the next item location is only returned if it is within the payload.
 *
 * @param[in] sdata The serial data sourcev
 * @param[in] item The current element we are travering from
 * @return The next item or NULL if there are no more items
 */
static struct item *
getnextitem(serial_data_t sdata, struct item *item) {
	struct item *next_item
			= (struct item *)PTR_UOFFSET(item, getitemsizetotal(item));
	/* if we should check item bounds against sdata */
	if(sdata) {
		if(OFF_PTRDIFF(next_item, sdata->payload) >= sdata->payload_size) {
			return NULL;
		}
	}
	return next_item;
}

/**@internal
 * @brief Fetch a pointer to the info table
 * @param[in] sdata The serial data source
 * @return Pointer to the inner info table, or NULL if table is disabled.
 */
static struct item_info_table *
gettable(serial_data_t sdata) {
	assert(sdata);
	if(sdata->type != SERIAL_TYPE_WITHTABLE) {
		return NULL;
	}
	return (struct item_info_table *)sdata->payload;
}

/**@internal
 * @brief Fetch the info from the table for a particular item index
 * @param[in] sdata The serial data source
 * @param[in] index The item's index
 * @return The item's info entry or NULL if no table exists
 */
static struct item_info *
getinfo(serial_data_t sdata, size_t index) {
	struct item_info_table *table;
	assert(sdata);
	table = gettable(sdata);
	if(table) {
		return &table->info[index];
	} else {
		return NULL;
	}
}

/**@internal
 * @brief Fetch the index*th item
 *
 * We traverse serial data payload and find the index*th data item struct.
 * We use the info table for a quick lookup, if the serial data contains
 * an info table.
 *
 * This operation is worst case O(n) for n elements.
 * If \ref SERIAL_TYPE_WITHTABLE is enabled, the worst case is O(1).
 *
 * @param[in] sdata The serial data source
 * @param[in] index The index of the item to get
 * @return
 */
static struct item *
getitem(serial_data_t sdata, size_t index) {
	assert(sdata);

	if(sdata->type == SERIAL_TYPE_WITHTABLE) {
		size_t payload_off;
		// ensure index is within the table
		assert(index < gettable(sdata)->count);
		payload_off = gettable(sdata)->info[index].payload_off;
		return (struct item *)PTR_UOFFSET(sdata->payload, payload_off);
	} else {
		struct item *item;
		size_t i;
		/* traverse index items */
		item = (struct item *)PTR_UOFFSET(sdata->payload, sdata->data_items_off);
		for(i = 0; i != index; i++) {
			item = getnextitem(sdata, item);
		}
		return item;
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

	assert(type==SERIAL_TYPE_BARE || type==SERIAL_TYPE_WITHTABLE);

	/* count number of items and their sizes */
	item_count = 0;
	va_copy(ap, va);
	fmt_ch = fmt;
	for(fmt_ch = fmt; *fmt_ch; fmt_ch++) {
		item_count++;
		payload_total_size += sizeof(struct item);
		if(IS_TYPE_ARRAY(*fmt_ch)) {
			payload_total_size += va_arg(ap, size_t);
			va_arg(ap, void *);
		} else {
			// jog the va list to stay in line for arrays
			switch(*fmt_ch) {
			case DATA_TYPE_CHAR:     va_arg(ap, int); break;
			case DATA_TYPE_SHORT:    va_arg(ap, int); break;
			case DATA_TYPE_INT:      va_arg(ap, int); break;
			case DATA_TYPE_LONG:     va_arg(ap, long); break;
			case DATA_TYPE_LONGLONG: va_arg(ap, long long); break;
			default: assert(0);
			}
		}
	}
	va_end(ap);
	/* account for the table's space requirement */
	if(type == SERIAL_TYPE_WITHTABLE) {
		table_total_size = sizeof(struct item_info_table);
		table_total_size += sizeof(struct item_info)*item_count;
		payload_total_size += table_total_size;
	}

	/* allocate serial data */
	sdata = (serial_data_t)malloc(sizeof(struct serial_data)+payload_total_size);
	if(sdata == NULL) {
		perror("couldn't allocate a serial_data_t");
	}
	sdata->type = type;
	sdata->data_items_off = 0;
	sdata->payload_size = payload_total_size;

	/* fill in table */
	if(type == SERIAL_TYPE_WITHTABLE) {
		table = (struct item_info_table *)sdata->payload;
		sdata->data_items_off = table_total_size;
		table->count = item_count;
	}

	/* copy in items and setup info table */
	item_index = 0;
	item = (struct item *)(sdata->payload+sdata->data_items_off);
	va_copy(ap, va);
	for(fmt_ch = fmt; *fmt_ch; fmt_ch++) {
		size_t item_payload_off = OFF_PTRDIFF(item, sdata->payload);
		size_t item_size_total = sizeof(struct item);

		/* handle copying in an array */
		if(IS_TYPE_ARRAY(*fmt_ch)) {
			size_t buf_size = va_arg(ap, size_t);
			void  *buf      = va_arg(ap, void *);

			assert(buf_size > 0);
			assert(buf);

			item->data.array.buf_size = buf_size;
			memcpy(item->data.array.buf, buf, buf_size);

			item_size_total += buf_size;

		}
		/* handle copying in a primitive data type */
		else {
			switch(*fmt_ch) {
			case DATA_TYPE_CHAR:     item->data.prim.CHAR=va_arg(ap, int); break;
			case DATA_TYPE_SHORT:    item->data.prim.SHORT=va_arg(ap, int); break;
			case DATA_TYPE_INT:      item->data.prim.INT=va_arg(ap, int); break;
			case DATA_TYPE_LONG:     item->data.prim.LONG=va_arg(ap, long); break;
			case DATA_TYPE_LONGLONG: item->data.prim.LONGLONG=va_arg(ap, long long); break;
			default: assert(0);
			}

		}

		/* set item type */
		item->type = *fmt_ch;

		/* set table entry */
		if(type==SERIAL_TYPE_WITHTABLE) {
			table->info[item_index].type = *fmt_ch;
			table->info[item_index].payload_off = item_payload_off;
		}

		/* iterate to next item */
		item = getnextitem(sdata, item);

		item_index++;
	}
	va_end(ap);
	return sdata;
}

serial_data_t serial_pack_extra(serial_type_t type, const char *fmt, ...) {
	va_list ap;
	serial_data_t sdata;
	va_start(ap, fmt);
	sdata = serial_pack_vextra(type, fmt, ap);
	va_end(ap);
	return sdata;
}

serial_data_t serial_pack(const char *fmt, ...) {
	va_list ap;
	serial_data_t sdata;
	va_start(ap, fmt);
	sdata = serial_pack_vextra(SERIAL_TYPE_BARE, fmt, ap);
	va_end(ap);
	return sdata;
}

/**
 * @brief Set the iterator to the first item of the serial data
 * @param[in] sdata The reference serial data containing iterable items
 * @param[out] it The iterator to setup
 * @return \e it
 */
serial_item_iterator_t *
serial_iit_begin(serial_item_iterator_t *it, serial_data_t sdata) {
	assert(sdata);
	assert(it);
	it->sdata = sdata;
	it->item = getitem(sdata, 0);
	return it;
}

/**
 * @brief Progress the iterator to the next item
 * @param[in] it The
 * @return \e it or NULL if no more items exist
 */
serial_item_iterator_t *
serial_iit_next(serial_item_iterator_t *it) {
	assert(it);
	it->item = getnextitem(it->sdata, it->item);
	return it->item?it:NULL;
}
int serial_iit_hasnext(serial_item_iterator_t *it) {
	assert(it);
	return it->item?(getnextitem(it->sdata, it->item)!=NULL):0;
}
int serial_iit_isend(serial_item_iterator_t *it) {
	assert(it);
	return it->item == NULL;
}

char serial_iit_get_char(serial_item_iterator_t *it) {
	assert(it);
	return it->item?it->item->data.prim.CHAR:0;
}
short serial_iit_get_short(serial_item_iterator_t *it) {
	assert(it);
	return it->item?it->item->data.prim.SHORT:0;
}
int serial_iit_get_int(serial_item_iterator_t *it) {
	assert(it);
	return it->item?it->item->data.prim.INT:0;
}
const void *serial_iit_get_buf_ptr(serial_item_iterator_t *it) {
	assert(it);
	return it->item?it->item->data.array.buf:NULL;
}
size_t serial_iit_get_buf(serial_item_iterator_t *it, void *buf) {
	size_t buf_size = 0;
	assert(it);
	assert(buf);
	if(it->item) {
		buf_size = serial_iit_get_size(it);
		memcpy(buf, it->item->data.array.buf, buf_size);
	}
	return buf_size;
}
size_t serial_iit_get_size(serial_item_iterator_t *it) {
	assert(it);
	return it->item?getitemsize(it->item):0;
}
data_type_t serial_iit_get_type(serial_item_iterator_t *it) {
	assert(it);
	return it->item?it->item->type:0;
}

size_t serial_data_size(serial_data_t sdata) {
	assert(sdata);
	return sizeof(struct serial_data) + sdata->payload_size;
}

void *serial_data_flat_ptr(serial_data_t sdata) {
	assert(sdata);
	return sdata;
}

/**
 * @brief Retrieve the number of items in the serial data
 * @param[in] sdata The serial data source
 * @return The number of items in the serial data
 */
size_t serial_item_count(serial_data_t sdata) {
	assert(sdata);

	if(sdata->type==SERIAL_TYPE_WITHTABLE) {
		return gettable(sdata)->count;
	} else {
		struct item *item;
		size_t item_count = 0;
		/* traverse items */
		for(item = getitem(sdata, 0); item; item = getnextitem(sdata, item)) {
			item_count++;
		}
		return item_count;
	}
}

int serial_item_get_int(serial_data_t sdata, size_t index) {
	struct item *item;
	assert(sdata);

	item = getitem(sdata, index);
	return item->data.prim.INT;
}

void serial_print_table(serial_data_t sdata) {
	struct item_info_table *table;
	size_t index;
	table = gettable(sdata);
	if(table == NULL) {
		printf("[ no table ]\n");
		return;
	}

	for(index = 0; index < table->count; index++) {
		struct item_info info = table->info[index];
		printf("[%lu] type: %c | off: %lu\n", index, info.type, info.payload_off);
	}
}

void serial_print_items(serial_data_t sdata) {
	struct item *item;
	size_t item_index = 0;

	/* traverse items */
	for(item = getitem(sdata, 0); item; item = getnextitem(sdata, item)) {
		printf("[%lu] %c | ", item_index, item->type);
		if(IS_TYPE_ARRAY(item->type)) {
			printf("[size: %lu]\n", item->data.array.buf_size);
		} else {
			switch(item->type) {
			case DATA_TYPE_CHAR:     printf("%c\n", item->data.prim.CHAR); break;
			case DATA_TYPE_SHORT:    printf("%hu\n", item->data.prim.SHORT); break;
			case DATA_TYPE_INT:      printf("%u\n", item->data.prim.INT); break;
			case DATA_TYPE_LONG:     printf("%lu\n", item->data.prim.LONG); break;
			case DATA_TYPE_LONGLONG: printf("%llu\n", item->data.prim.LONGLONG); break;
			default: assert(0);
			}
		}
		item_index++;
	}
}
