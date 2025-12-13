#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

// Graph: map string -> adjacency list
// Implemented via a simple hash table of nodes, each node has a dynamic vector of edges.
typedef struct {
    char  *key;      // node name
    int   *nbr;      // neighbor indices
    int    nbr_n;
    int    nbr_cap;
    bool   used;
} Node;

typedef struct {
    Node  *tab;
    int    cap;
    int    n;
} Graph;

static uint64_t
hash_str(const char *s)
{
    // FNV-1a 64-bit
    uint64_t h = 1469598103934665603ULL;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        h ^= (uint64_t)(*p);
        h *= 1099511628211ULL;
    }
    return h;
}

static void
graph_init(Graph *g, int cap)
{
    g->cap = cap;
    g->n = 0;
    g->tab = (Node *)calloc((size_t)cap, sizeof(Node));
    if (g->tab == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(EXIT_FAILURE);
    }
}

static void
node_push_edge(Node *nd, int v)
{
    if (nd->nbr_n == nd->nbr_cap) {
        int nc = (nd->nbr_cap == 0) ? 4 : nd->nbr_cap * 2;
        int *p = (int *)realloc(nd->nbr, (size_t)nc * sizeof(int));
        if (p == NULL) {
            fprintf(stderr, "out of memory\n");
            exit(EXIT_FAILURE);
        }
        nd->nbr = p;
        nd->nbr_cap = nc;
    }
    nd->nbr[nd->nbr_n++] = v;
}

static void graph_rehash(Graph *g);

static int
graph_intern(Graph *g, const char *key)
{
    if (g->n * 10 >= g->cap * 7) { // load factor ~0.7
        graph_rehash(g);
    }

    uint64_t h = hash_str(key);
    int mask = g->cap - 1;
    int i = (int)(h & (uint64_t)mask);

    for (;;) {
        Node *nd = &g->tab[i];
        if (!nd->used) {
            nd->used = true;
            nd->key = xstrdup(key);
            nd->nbr = NULL;
            nd->nbr_n = 0;
            nd->nbr_cap = 0;
            g->n++;
            return i;
        }
        if (strcmp(nd->key, key) == 0) {
            return i;
        }
        i = (i + 1) & mask;
    }
}

static int
graph_find(const Graph *g, const char *key)
{
    if (g->cap == 0) return -1;
    uint64_t h = hash_str(key);
    int mask = g->cap - 1;
    int i = (int)(h & (uint64_t)mask);

    for (;;) {
        const Node *nd = &g->tab[i];
        if (!nd->used) return -1;
        if (strcmp(nd->key, key) == 0) return i;
        i = (i + 1) & mask;
    }
}

static void
graph_rehash(Graph *g)
{
    int old_cap = g->cap;
    Node *old = g->tab;

    int new_cap = (old_cap == 0) ? 1024 : old_cap * 2;
    // keep cap power-of-two for mask trick
    if ((new_cap & (new_cap - 1)) != 0) {
        // bump to next power of two
        int p = 1;
        while (p < new_cap) p <<= 1;
        new_cap = p;
    }

    Graph ng;
    graph_init(&ng, new_cap);

    for (int i = 0; i < old_cap; i++) {
        Node *nd = &old[i];
        if (!nd->used) continue;

        int idx = graph_intern(&ng, nd->key);

        // Move adjacency
        ng.tab[idx].nbr = nd->nbr;
        ng.tab[idx].nbr_n = nd->nbr_n;
        ng.tab[idx].nbr_cap = nd->nbr_cap;

        // Steal key ownership
        free(ng.tab[idx].key);
        ng.tab[idx].key = nd->key;

        nd->key = NULL;
        nd->nbr = NULL;
        nd->nbr_n = nd->nbr_cap = 0;
        nd->used = false;
    }

    free(old);
    *g = ng;
}

static void
graph_free(Graph *g)
{
    for (int i = 0; i < g->cap; i++) {
        if (g->tab[i].used) {
            free(g->tab[i].key);
            free(g->tab[i].nbr);
        }
    }
    free(g->tab);
    g->tab = NULL;
    g->cap = 0;
    g->n = 0;
}

// Memoization table: (node_index, mask) -> uint64
// mask is 2 bits => 4 states per node. We can store in arrays.
// visiting: detects cycles per (node,mask) recursion stack.
typedef struct {
    uint64_t *memo;    // size = cap*4, only valid where has_memo is true
    uint8_t  *has;     // size = cap*4, 0/1
    uint8_t  *vis;     // size = cap*4, 0/1
    int       cap;     // same as graph cap
} DP;

static inline int
dp_ix(int node_index, uint8_t mask)
{
    return (node_index << 2) | (int)(mask & 3U);
}

static void
read_graph(Graph *g, FILE *fp)
{
    graph_init(g, 1024); // power-of-two-ish; will rehash as needed

    char line[8192];
    while (fgets(line, (int)sizeof line, fp) != NULL) {
        // strip newline
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) {
            line[--n] = '\0';
        }

        char *s = trim_inplace(line);
        if (*s == '\0') continue;

        char *colon = strchr(s, ':');
        if (colon == NULL) continue;

        *colon = '\0';
        char *src = trim_inplace(s);
        char *rhs = trim_inplace(colon + 1);
        if (*src == '\0') continue;

        int src_i = graph_intern(g, src);

        if (*rhs == '\0') continue;

        // tokenize rhs
        char *tok[2048];
        int nt = split_fields(rhs, tok, (int)(sizeof tok / sizeof tok[0]));
        for (int i = 0; i < nt; i++) {
            if (tok[i] == NULL || tok[i][0] == '\0') continue;
            int dst_i = graph_intern(g, tok[i]);
            node_push_edge(&g->tab[src_i], dst_i);
        }
    }

    if (ferror(fp)) {
        fprintf(stderr, "read error\n");
        exit(EXIT_FAILURE);
    }
}

// DFS w/ memoization
static uint64_t
count_paths_with_devices(const Graph *g, DP *dp,
                         int node, int target, int dac, int fft,
                         uint8_t mask)
{
    if (node == dac) mask |= 1U << 0;
    if (node == fft) mask |= 1U << 1;

    if (node == target) {
        return (mask == 3U) ? 1ULL : 0ULL;
    }

    int idx = dp_ix(node, mask);

    if (dp->has[idx]) {
        return dp->memo[idx];
    }

    if (dp->vis[idx]) {
        // cycle on current recursion stack => do not count infinite paths
        return 0ULL;
    }

    dp->vis[idx] = 1U;

    uint64_t total = 0;
    const Node *nd = &g->tab[node];
    for (int i = 0; i < nd->nbr_n; i++) {
        int next = nd->nbr[i];
        total += count_paths_with_devices(g, dp, next, target, dac, fft, mask);
    }

    dp->vis[idx] = 0U;
    dp->has[idx] = 1U;
    dp->memo[idx] = total;
    return total;
}

int
main(void)
{
    Graph g = {0};
    read_graph(&g, stdin);

    const char *start_s  = "svr";
    const char *target_s = "out";
    const char *dac_s    = "dac";
    const char *fft_s    = "fft";

    int start  = graph_find(&g, start_s);
    if (start < 0) {
        puts("0");
        graph_free(&g);
        return 0;
    }

    int target = graph_find(&g, target_s);
    int dac    = graph_find(&g, dac_s);
    int fft    = graph_find(&g, fft_s);

    // If target doesn't exist, there are no paths.
    if (target < 0) {
        puts("0");
        graph_free(&g);
        return 0;
    }

    DP dp = {0};
    dp.cap = g.cap;
    size_t sz = (size_t)g.cap * 4U;

    dp.memo = (uint64_t *)calloc(sz, sizeof(uint64_t));
    dp.has  = (uint8_t  *)calloc(sz, sizeof(uint8_t));
    dp.vis  = (uint8_t  *)calloc(sz, sizeof(uint8_t));
    if (dp.memo == NULL || dp.has == NULL || dp.vis == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(EXIT_FAILURE);
    }

    uint64_t paths = count_paths_with_devices(&g, &dp, start, target, dac, fft, 0);
    printf("%" PRIu64 "\n", paths);

    free(dp.memo);
    free(dp.has);
    free(dp.vis);
    graph_free(&g);
    return 0;
}
