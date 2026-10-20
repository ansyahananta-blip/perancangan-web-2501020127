#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define NIM_LEN   32
#define NAMA_LEN  128
#define LINE_LEN  512
#define HASH_SIZE 1024

typedef struct {
    char nim[NIM_LEN];
    char nama[NAMA_LEN];
    char nama_lc[NAMA_LEN];
} Record;

typedef struct HNode {
    int idx;
    struct HNode *next;
} HNode;

static Record *data = NULL;
static int count = 0, capacity = 0;
static HNode *table[HASH_SIZE];
static FILE *fp = NULL;

static void to_lower(char *dst, const char *src) {
    while (*src) *dst++ = (char)tolower((unsigned char)*src++);
    *dst = '\0';
}

static void trim(char *s) {
    char *p = s;
    while (isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
}

static int is_number(const char *s) {
    if (!*s) return 0;
    for (; *s; s++) if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

static int has_letter(const char *s) {
    for (; *s; s++) if (isalpha((unsigned char)*s)) return 1;
    return 0;
}

static unsigned hash(const char *s) {
    unsigned h = 5381;
    while (*s) h = h * 33 + (unsigned char)*s++;
    return h % HASH_SIZE;
}

static int nim_exists(const char *nim) {
    for (HNode *n = table[hash(nim)]; n; n = n->next)
        if (strcmp(data[n->idx].nim, nim) == 0) return 1;
    return 0;
}

static void hash_insert(int idx) {
    unsigned h = hash(data[idx].nim);
    HNode *n = malloc(sizeof(HNode));
    if (!n) { perror("malloc"); exit(1); }
    n->idx = idx;
    n->next = table[h];
    table[h] = n;
}

static void add_record(const char *nim, const char *nama) {
    if (count == capacity) {
        capacity = capacity ? capacity * 2 : 64;
        Record *tmp = realloc(data, capacity * sizeof(Record));
        if (!tmp) { perror("realloc"); exit(1); }
        data = tmp;
    }
    strncpy(data[count].nim, nim, NIM_LEN - 1);
    data[count].nim[NIM_LEN - 1] = '\0';
    strncpy(data[count].nama, nama, NAMA_LEN - 1);
    data[count].nama[NAMA_LEN - 1] = '\0';
    to_lower(data[count].nama_lc, data[count].nama);
    hash_insert(count);
    count++;
}

static void load_file(const char *filename) {
    fp = fopen(filename, "r");
    if (fp) {
        char line[LINE_LEN];
        while (fgets(line, sizeof line, fp)) {
            char *comma = strchr(line, ',');
            if (!comma) continue;
            *comma = '\0';
            char *nama = comma + 1;
            trim(line);
            trim(nama);
            if (*line && *nama && !nim_exists(line)) add_record(line, nama);
        }
        fclose(fp);
    }

    fp = fopen(filename, "a");
    if (!fp) {
        fprintf(stderr, "Tidak dapat membuka/membuat file %s\n", filename);
        exit(1);
    }
}

static void cmd_help(void) {
    printf("Available command:\n");
    printf("help - display this available\n");
    printf("berhenti - exit and return to Operating System\n");
    printf("cari xxxx yyyy - search xxxx by yyyyy value\n");
    printf("tambah xxxx yyyy - add new data xxxx and yyyyy\n");
}

static void cmd_tambah(char *args) {
    char *nim = args;
    while (*nim && isspace((unsigned char)*nim)) nim++;
    char *p = nim;
    while (*p && !isspace((unsigned char)*p)) p++;
    char *nama = p;
    if (*p) { *p = '\0'; nama = p + 1; }
    trim(nama);

    if (!*nim || !*nama || !is_number(nim) || !has_letter(nama) || strchr(nama, ',')) {
        printf("Parameter error. tambah xxxx yyyy where xxxx is number and yyy is string\n");
        return;
    }
    if (strlen(nim) >= NIM_LEN || strlen(nama) >= NAMA_LEN) {
        printf("Parameter error. tambah xxxx yyyy where xxxx is number and yyy is string\n");
        return;
    }
    if (nim_exists(nim)) {
        printf("NIM %s sudah ada. Data tidak ditambahkan.\n", nim);
        return;
    }
    add_record(nim, nama);
    fprintf(fp, "%s,%s\n", nim, nama);
    fflush(fp);
}

static void syntax_error_cari(void) {
    printf("Syntax error. Should only\n");
    printf("cari xxx yyy\n");
    printf("Where xxx is NIM or NAMA, and yyy is the search value\n");
}

static void cmd_cari(char *args) {
    char *field = args;
    while (*field && isspace((unsigned char)*field)) field++;
    char *p = field;
    while (*p && !isspace((unsigned char)*p)) p++;
    char *value = p;
    if (*p) { *p = '\0'; value = p + 1; }
    trim(value);

    int by_nim;
    if (strcmp(field, "NIM") == 0) by_nim = 1;
    else if (strcmp(field, "NAMA") == 0) by_nim = 0;
    else { syntax_error_cari(); return; }
    if (!*value) { syntax_error_cari(); return; }

    char key[LINE_LEN];
    to_lower(key, value);

    clock_t start = clock();
    printf("Result:\n");
    for (int i = 0; i < count; i++) {
        const char *target = by_nim ? data[i].nim : data[i].nama_lc;
        if (strstr(target, key)) printf("%s %s\n", data[i].nim, data[i].nama);
    }
    double sec = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Total time %.3f seconds\n", sec);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Pemakaian: %s namafile.dat\n", argv[0]);
        return 1;
    }
    load_file(argv[1]);

    char line[LINE_LEN];
    for (;;) {
        printf("Your command: ");
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) break;
        trim(line);
        if (!*line) continue;

        char *args = line;
        while (*args && !isspace((unsigned char)*args)) args++;
        if (*args) *args++ = '\0';

        if (strcmp(line, "help") == 0) cmd_help();
        else if (strcmp(line, "berhenti") == 0) break;
        else if (strcmp(line, "tambah") == 0) cmd_tambah(args);
        else if (strcmp(line, "cari") == 0) cmd_cari(args);
        else printf("Command not recognised. Please use help to display list of command.\n");
    }

    fclose(fp);
    for (int i = 0; i < HASH_SIZE; i++)
        for (HNode *n = table[i]; n; ) { HNode *t = n->next; free(n); n = t; }
    free(data);
    return 0;
}