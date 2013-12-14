#include "common.h"
#include "noise.h"
#include "world.h"

void create_world(Map *map, int p, int q) {
    int pad = 1, dx, dz;
    for (dx = -pad; dx < CHUNK_SIZE + pad; dx++) {
        for (dz = -pad; dz < CHUNK_SIZE + pad; dz++) {
            int ok, x, y, z, mh, h, w, t;
            float f, g;
            x = p * CHUNK_SIZE + dx;
            z = q * CHUNK_SIZE + dz;
            f = simplex2(x * 0.01, z * 0.01, 4, 0.5, 2);
            g = simplex2(-x * 0.01, -z * 0.01, 2, 0.9, 2);
            mh = g * 32 + 16;
            h = f * mh;
            w = 1;
            t = 12;
            if (h <= t) {
                h = t;
                w = 2;
            }
            if (dx < 0 || dz < 0 || dx >= CHUNK_SIZE || dz >= CHUNK_SIZE) {
                w = -1;
            }
            // sand and grass terrain
            for (y = 0; y < h; y++) {
                map_set(map, x, y, z, w);
            }
            // TODO: w = -1 if outside of chunk
            if (w == 1) {
                // grass
                if (simplex2(-x * 0.1, z * 0.1, 4, 0.8, 2) > 0.6) {
                    map_set(map, x, h, z, 17);
                }
                // flowers
                if (simplex2(x * 0.05, -z * 0.05, 4, 0.8, 2) > 0.7) {
                    int w = 18 + simplex2(x * 0.1, z * 0.1, 4, 0.8, 2) * 7;
                    map_set(map, x, h, z, w);
                }
                // trees
                ok = 1;
                if (dx - 4 < 0 || dz - 4 < 0 ||
                    dx + 4 >= CHUNK_SIZE || dz + 4 >= CHUNK_SIZE)
                {
                    ok = 0;
                }
                if (ok && simplex2(x, z, 6, 0.5, 2) > 0.84) {
                    for (y = h + 3; y < h + 8; y++) {
                        int ox;
                        for (ox = -3; ox <= 3; ox++) {
                            int oz;
                            for (oz = -3; oz <= 3; oz++) {
                                int d = (ox * ox) + (oz * oz) +
                                    (y - (h + 4)) * (y - (h + 4));
                                if (d < 11) {
                                    map_set(map, x + ox, y, z + oz, 15);
                                }
                            }
                        }
                    }
                    for (y = h; y < h + 7; y++) {
                        map_set(map, x, y, z, 5);
                    }
                }
            }
            // clouds
            for (y = 64; y < 72; y++) {
                if (simplex3(x * 0.01, y * 0.1, z * 0.01, 8, 0.5, 2) > 0.75) {
                    map_set(map, x, y, z, 16);
                }
            }
        }
    }
}
