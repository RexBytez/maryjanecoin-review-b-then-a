#include <stdio.h>
#include <assert.h>
#include "../include/minisketch.h"

int main(void) {

  minisketch *sketch_a = minisketch_create(12, 0, 4);

  for (int i = 3000; i < 3010; ++i) {
    minisketch_add_uint64(sketch_a, i);
  }

  size_t sersize = minisketch_serialized_size(sketch_a);
  assert(sersize == 12 * 4 / 8);
  unsigned char *buffer_a = malloc(sersize);
  minisketch_serialize(sketch_a, buffer_a);
  minisketch_destroy(sketch_a);

  minisketch *sketch_b = minisketch_create(12, 0, 4);
  for (int i = 3002; i < 3012; ++i) {
    minisketch_add_uint64(sketch_b, i);
  }

  sketch_a = minisketch_create(12, 0, 4);
  minisketch_deserialize(sketch_a, buffer_a);
  free(buffer_a);

  minisketch_merge(sketch_b, sketch_a);

  uint64_t differences[4];
  ssize_t num_differences = minisketch_decode(sketch_b, 4, differences);
  minisketch_destroy(sketch_a);
  minisketch_destroy(sketch_b);
  if (num_differences < 0) {
    printf("More than 4 differences!\n");
  } else {
    ssize_t i;
    for (i = 0; i < num_differences; ++i) {
      printf("%u is in only one of the two sets\n", (unsigned)differences[i]);
    }
  }
}
