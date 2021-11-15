test: test.c lru_cache.h lru_cache_impl.h lru_cache_impl.c
	gcc -o test test.c lru_cache.h lru_cache_impl.h lru_cache_impl.c -lpthread

example: example.c lru_cache.h lru_cache_impl.h lru_cache_impl.c
	gcc -o example example.c lru_cache.h lru_cache_impl.h lru_cache_impl.c -lpthread

clean:
	rm test example