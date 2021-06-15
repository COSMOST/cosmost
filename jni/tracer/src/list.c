
static void add_list_tail(buf_t *buf)
{
	buf_t *prev;
	while (1) {
		prev = head.prev;
		buf->prev = prev;
		if (__sync_bool_compare_and_swap(&head.prev, prev, buf))	 {
			// prev is safe by refcnt held during init
			if (prev)
				__sync_fetch_and_sub(&prev->idx_refcnt, 1);
			break;
		}
	}
}
