/*
 * inventory.c – Retail Store Inventory Management System
 * Standard : C11
 * Compile  : gcc -std=c11 -Wall -Wextra -o inventory inventory.c
 * Run      : ./inventory
 *
 * _POSIX_C_SOURCE 200809L is defined first to expose strcasecmp()
 * from <strings.h> (a POSIX function, not in strict C11).
 *
 * inventory.txt format (CSV, one item per line):
 *   name,quantity,price
 *   Apple,100,0.99
 *   # Lines starting with '#' are comments and are ignored.
 */
#define _POSIX_C_SOURCE 200809L

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>   /* strcasecmp (POSIX) */
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>

/* ─── Constants ─────────────────────────────────────────────── */
#define MAX_ITEMS       500
#define MAX_NAME_LEN    64
#define INVENTORY_FILE  "inventory.txt"
#define LINE_BUF        256

/* ─── Data structure ─────────────────────────────────────────── */
typedef struct {
    char   name[MAX_NAME_LEN]; /* product name              */
    int    quantity;           /* units in stock            */
    double price;              /* unit price (currency)     */
} Item;

/* ─── Global store ───────────────────────────────────────────── */
static Item g_items[MAX_ITEMS]; /* in-memory item array */
static int  g_count = 0;        /* current number of items */

/* ══════════════════════════════════════════════════════════════
 *  Utility helpers
 * ══════════════════════════════════════════════════════════════ */

/* Trim leading and trailing whitespace in-place. */
static void trim(char *s) {
    if (!s) return;
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    memmove(s, p, strlen(p) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';
}

/* Case-insensitive linear search. Returns index, or -1 if absent. */
static int find_item(const char *name) {
    for (int i = 0; i < g_count; i++)
        if (strcasecmp(g_items[i].name, name) == 0)
            return i;
    return -1;
}

/* ══════════════════════════════════════════════════════════════
 *  File I/O
 * ══════════════════════════════════════════════════════════════ */

/*
 * load_inventory
 *   Reads CSV rows from INVENTORY_FILE into g_items[].
 *   A missing file is treated as an empty inventory (not an error).
 *   Returns true on success.
 */
static bool load_inventory(void) {
    FILE *fp = fopen(INVENTORY_FILE, "r");
    if (!fp) {
        if (errno == ENOENT) {
            printf("[INFO] '%s' not found – starting with empty inventory.\n",
                   INVENTORY_FILE);
            return true;
        }
        fprintf(stderr, "[ERROR] Cannot open '%s': %s\n",
                INVENTORY_FILE, strerror(errno));
        return false;
    }

    char line[LINE_BUF];
    int  lineno = 0;
    g_count = 0;

    while (fgets(line, sizeof line, fp)) {
        lineno++;
        trim(line);

        /* Skip blank lines and comments */
        if (line[0] == '\0' || line[0] == '#') continue;

        if (g_count >= MAX_ITEMS) {
            fprintf(stderr, "[WARN] Max capacity (%d) reached; remaining lines ignored.\n",
                    MAX_ITEMS);
            break;
        }

        /* Parse: name , quantity , price */
        char tmp[LINE_BUF];
        strncpy(tmp, line, LINE_BUF - 1);
        tmp[LINE_BUF - 1] = '\0';

        char *tok_name  = strtok(tmp,  ",");
        char *tok_qty   = strtok(NULL, ",");
        char *tok_price = strtok(NULL, ",");

        if (!tok_name || !tok_qty || !tok_price) {
            fprintf(stderr, "[WARN] Line %d: malformed record (skipped): %s\n",
                    lineno, line);
            continue;
        }

        trim(tok_name); trim(tok_qty); trim(tok_price);

        /* Validate name length */
        if (strlen(tok_name) == 0 || strlen(tok_name) >= MAX_NAME_LEN) {
            fprintf(stderr, "[WARN] Line %d: invalid name length (skipped).\n", lineno);
            continue;
        }

        /* Validate quantity (non-negative integer) */
        char *ep;
        long lqty = strtol(tok_qty, &ep, 10);
        if (*ep != '\0' || lqty < 0 || lqty > 1000000) {
            fprintf(stderr, "[WARN] Line %d: invalid quantity '%s' (skipped).\n",
                    lineno, tok_qty);
            continue;
        }

        /* Validate price (non-negative float) */
        double price = strtod(tok_price, &ep);
        if (*ep != '\0' || price < 0.0 || price > 1e9) {
            fprintf(stderr, "[WARN] Line %d: invalid price '%s' (skipped).\n",
                    lineno, tok_price);
            continue;
        }

        /* Skip duplicates */
        if (find_item(tok_name) >= 0) {
            fprintf(stderr, "[WARN] Line %d: duplicate name '%s' (skipped).\n",
                    lineno, tok_name);
            continue;
        }

        /* Commit record */
        strncpy(g_items[g_count].name, tok_name, MAX_NAME_LEN - 1);
        g_items[g_count].name[MAX_NAME_LEN - 1] = '\0';
        g_items[g_count].quantity = (int)lqty;
        g_items[g_count].price    = price;
        g_count++;
    }

    fclose(fp);
    printf("[INFO] Loaded %d item(s) from '%s'.\n", g_count, INVENTORY_FILE);
    return true;
}

/*
 * save_inventory
 *   Overwrites INVENTORY_FILE with the current in-memory state.
 *   Returns true on success.
 */
static bool save_inventory(void) {
    FILE *fp = fopen(INVENTORY_FILE, "w");
    if (!fp) {
        fprintf(stderr, "[ERROR] Cannot write '%s': %s\n",
                INVENTORY_FILE, strerror(errno));
        return false;
    }

    fprintf(fp, "# Retail Inventory – format: name,quantity,price\n");
    for (int i = 0; i < g_count; i++)
        fprintf(fp, "%s,%d,%.2f\n",
                g_items[i].name,
                g_items[i].quantity,
                g_items[i].price);

    fclose(fp);
    printf("[INFO] %d item(s) saved to '%s'.\n", g_count, INVENTORY_FILE);
    return true;
}

/* ══════════════════════════════════════════════════════════════
 *  Core inventory operations
 * ══════════════════════════════════════════════════════════════ */

/*
 * add_item
 *   If the item already exists its stock is incremented and its price updated.
 *   Otherwise a new record is created.
 */
static bool add_item(const char *name, int qty, double price) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_NAME_LEN) {
        fprintf(stderr, "[ERROR] Invalid item name.\n"); return false;
    }
    if (qty <= 0)   { fprintf(stderr, "[ERROR] Quantity must be > 0.\n");         return false; }
    if (price < 0)  { fprintf(stderr, "[ERROR] Price cannot be negative.\n");      return false; }

    int idx = find_item(name);
    if (idx >= 0) {
        /* Restock existing item */
        g_items[idx].quantity += qty;
        g_items[idx].price     = price;
        printf("[OK] Restocked '%s' → qty=%d, price=%.2f\n",
               g_items[idx].name, g_items[idx].quantity, g_items[idx].price);
        return true;
    }

    if (g_count >= MAX_ITEMS) {
        fprintf(stderr, "[ERROR] Inventory full (max %d items).\n", MAX_ITEMS);
        return false;
    }

    strncpy(g_items[g_count].name, name, MAX_NAME_LEN - 1);
    g_items[g_count].name[MAX_NAME_LEN - 1] = '\0';
    g_items[g_count].quantity = qty;
    g_items[g_count].price    = price;
    g_count++;

    printf("[OK] Added '%s': qty=%d, price=%.2f\n", name, qty, price);
    return true;
}

/*
 * remove_item
 *   Deletes an item entirely from the store. Fills the gap by shifting
 *   subsequent elements left (order-preserving).
 */
static bool remove_item(const char *name) {
    int idx = find_item(name);
    if (idx < 0) {
        fprintf(stderr, "[ERROR] '%s' not found in inventory.\n", name);
        return false;
    }
    for (int i = idx; i < g_count - 1; i++)
        g_items[i] = g_items[i + 1];
    g_count--;
    printf("[OK] Removed '%s'.\n", name);
    return true;
}

/*
 * update_quantity
 *   Sets an item's stock to an absolute value (>= 0).
 *   Passing 0 effectively marks the item as out-of-stock.
 */
static bool update_quantity(const char *name, int new_qty) {
    if (new_qty < 0) { fprintf(stderr, "[ERROR] Quantity cannot be negative.\n"); return false; }
    int idx = find_item(name);
    if (idx < 0) {
        fprintf(stderr, "[ERROR] '%s' not found in inventory.\n", name);
        return false;
    }
    g_items[idx].quantity = new_qty;
    printf("[OK] '%s' quantity → %d\n", name, new_qty);
    return true;
}

/*
 * calculate_total
 *   Returns the sum of (quantity × price) for every item in stock.
 */
static double calculate_total(void) {
    double total = 0.0;
    for (int i = 0; i < g_count; i++)
        total += (double)g_items[i].quantity * g_items[i].price;
    return total;
}

/*
 * list_inventory
 *   Prints a formatted table with a running total row.
 */
static void list_inventory(void) {
    if (g_count == 0) { printf("  (inventory is empty)\n"); return; }

    const char *sep =
        "  ─────────────────────────────────────────────────────────────────\n";
    printf("\n  %-30s %8s %10s %14s\n", "Name", "Qty", "Price ($)", "Value ($)");
    printf("%s", sep);
    for (int i = 0; i < g_count; i++) {
        double val = (double)g_items[i].quantity * g_items[i].price;
        printf("  %-30s %8d %10.2f %14.2f\n",
               g_items[i].name, g_items[i].quantity, g_items[i].price, val);
    }
    printf("%s", sep);
    printf("  %-30s %8s %10s %14.2f\n\n", "TOTAL", "", "", calculate_total());
}

/* ══════════════════════════════════════════════════════════════
 *  Input helpers
 * ══════════════════════════════════════════════════════════════ */

/* Read a trimmed line from stdin. Returns false on EOF. */
static bool read_line(const char *prompt, char *buf, size_t n) {
    printf("%s", prompt); fflush(stdout);
    if (!fgets(buf, (int)n, stdin)) return false;
    buf[strcspn(buf, "\n")] = '\0';
    trim(buf);
    return true;
}

/* Parse a non-negative integer. Returns false on bad input. */
static bool parse_int(const char *s, int *out) {
    char *ep;
    long v = strtol(s, &ep, 10);
    if (*ep != '\0' || v < 0 || v > 1000000) return false;
    *out = (int)v; return true;
}

/* Parse a non-negative double. Returns false on bad input. */
static bool parse_double(const char *s, double *out) {
    char *ep;
    double v = strtod(s, &ep);
    if (*ep != '\0' || v < 0.0 || v > 1e9) return false;
    *out = v; return true;
}

/* ─── Individual menu actions ─────────────────────────────────── */

static void menu_add(void) {
    char   name[MAX_NAME_LEN], buf[64];
    int    qty;  double price;

    if (!read_line("  Item name  : ", name, sizeof name) || !name[0])
        { printf("[WARN] Cancelled.\n"); return; }
    if (!read_line("  Quantity   : ", buf, sizeof buf) || !parse_int(buf, &qty) || qty <= 0)
        { printf("[WARN] Invalid quantity – cancelled.\n"); return; }
    if (!read_line("  Price ($)  : ", buf, sizeof buf) || !parse_double(buf, &price))
        { printf("[WARN] Invalid price – cancelled.\n"); return; }

    add_item(name, qty, price);
}

static void menu_remove(void) {
    char name[MAX_NAME_LEN];
    if (!read_line("  Item name to remove: ", name, sizeof name) || !name[0])
        { printf("[WARN] Cancelled.\n"); return; }
    remove_item(name);
}

static void menu_update_qty(void) {
    char name[MAX_NAME_LEN], buf[64]; int qty;
    if (!read_line("  Item name    : ", name, sizeof name) || !name[0])
        { printf("[WARN] Cancelled.\n"); return; }
    if (!read_line("  New quantity : ", buf, sizeof buf) || !parse_int(buf, &qty))
        { printf("[WARN] Invalid quantity – cancelled.\n"); return; }
    update_quantity(name, qty);
}

static void menu_search(void) {
    char name[MAX_NAME_LEN];
    if (!read_line("  Search name: ", name, sizeof name) || !name[0])
        { printf("[WARN] Cancelled.\n"); return; }
    int idx = find_item(name);
    if (idx < 0) {
        printf("  Not found: '%s'\n", name);
    } else {
        printf("  %-30s qty=%-6d price=$%.2f  stock value=$%.2f\n",
               g_items[idx].name, g_items[idx].quantity,
               g_items[idx].price,
               (double)g_items[idx].quantity * g_items[idx].price);
    }
}

/* ══════════════════════════════════════════════════════════════
 *  main – interactive menu loop
 * ══════════════════════════════════════════════════════════════ */

int main(void) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    printf("╔══════════════════════════════════════════╗\n");
    printf("║   Retail Store Inventory Manager v1.0   ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    if (!load_inventory()) return EXIT_FAILURE;

    char choice[8];
    bool running = true;

    while (running) {
        printf("┌──────────────────────────────────────────┐\n");
        printf("│  1. List all items                       │\n");
        printf("│  2. Add / restock item                   │\n");
        printf("│  3. Remove item                          │\n");
        printf("│  4. Update quantity                      │\n");
        printf("│  5. Search item                          │\n");
        printf("│  6. Show total inventory value           │\n");
        printf("│  7. Save & exit                          │\n");
        printf("│  8. Exit without saving                  │\n");
        printf("└──────────────────────────────────────────┘\n");

        if (!read_line("Choice: ", choice, sizeof choice)) break;

        switch (choice[0]) {
            case '1': list_inventory();                                      break;
            case '2': menu_add();                                            break;
            case '3': menu_remove();                                         break;
            case '4': menu_update_qty();                                     break;
            case '5': menu_search();                                         break;
            case '6': printf("  Total inventory value: $%.2f\n",
                             calculate_total());                             break;
            case '7': save_inventory(); running = false;                     break;
            case '8': printf("[INFO] Exiting without saving.\n");
                      running = false;                                       break;
            default:  printf("[WARN] Unknown option '%s'. Try 1–8.\n", choice);
        }
    }

    return EXIT_SUCCESS;
}