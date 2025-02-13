/* Adds current and change, without overflowing the bounds of min and max */
static inline uint16_t saturating_add(uint16_t current, uint16_t min, uint16_t max, int16_t change) {
       if (0 == change)
               return current;
       else if (0 < change) {
               if (max - current < change) {
                       return max;
               } else {
                       return current + change;
               }
       } else {
               change = -change;
               if (current - min < change) {
                       return min;
               } else {
                       return current - change;
               }
       }
}
