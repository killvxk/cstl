/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **
 *  This file is part of cstl library
 *  Copyright (C) 2011 Avinash Dongre ( dongre.avinash@gmail.com )
 * 
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 * 
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

#include "c_lib.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define rb_sentinel &pTree->sentinel

static void debug_verify_properties(struct cstl_rb*);
static void debug_verify_property_1(struct cstl_rb*, struct cstl_rb_node*);
static void debug_verify_property_2(struct cstl_rb*, struct cstl_rb_node*);
static int debug_node_color(struct cstl_rb*, struct cstl_rb_node* n);
static void debug_verify_property_4(struct cstl_rb*, struct cstl_rb_node*);
static void debug_verify_property_5(struct cstl_rb*, struct cstl_rb_node*);
static void debug_verify_property_5_helper(struct cstl_rb*, struct cstl_rb_node*, int, int*);

static void
__left_rotate(struct cstl_rb* pTree, struct cstl_rb_node* x) {
    struct cstl_rb_node* y;
    y = x->right;
    x->right = y->left;
    if (y->left != rb_sentinel) {
        y->left->parent = x;
    }
    if (y != rb_sentinel) {
        y->parent = x->parent;
    }
    if (x->parent) {
        if (x == x->parent->left) {
            x->parent->left = y;
        } else {
            x->parent->right = y;
        }
    } else {
        pTree->root = y;
    }
    y->left = x;
    if (x != rb_sentinel) {
        x->parent = y;
    }
}

static void
__right_rotate(struct cstl_rb* pTree, struct cstl_rb_node* x) {
    struct cstl_rb_node* y = x->left;
    x->left = y->right;
    if (y->right != rb_sentinel) {
        y->right->parent = x;
    }
    if (y != rb_sentinel) {
        y->parent = x->parent;
    }
    if (x->parent) {
        if (x == x->parent->right) {
            x->parent->right = y;
        } else {
            x->parent->left = y;
        }
    } else {
        pTree->root = y;
    }
    y->right = x;
    if (x != rb_sentinel) {
        x->parent = y;
    }
}

struct cstl_rb*
cstl_rb_new(cstl_compare fn_c, cstl_destroy fn_ed, cstl_destroy fn_vd) {
    struct cstl_rb* pTree = (struct cstl_rb*)calloc(1, sizeof(struct cstl_rb));
    if (pTree == (struct cstl_rb*)0) {
        return (struct cstl_rb*)0;
    }
    pTree->compare_fn = fn_c;
    pTree->destruct_k_fn = fn_ed;
    pTree->destruct_v_fn = fn_vd;
    pTree->root = rb_sentinel;
    pTree->sentinel.left = rb_sentinel;
    pTree->sentinel.right = rb_sentinel;
    pTree->sentinel.parent = (struct cstl_rb_node*)0;
    pTree->sentinel.color = cstl_black;

    return pTree;
}

static void
__rb_insert_fixup(struct cstl_rb* pTree, struct cstl_rb_node* x) {
    while (x != pTree->root && x->parent->color == cstl_red) {
        if (x->parent == x->parent->parent->left) {
            struct cstl_rb_node* y = x->parent->parent->right;
            if (y->color == cstl_red) {
                x->parent->color = cstl_black;
                y->color = cstl_black;
                x->parent->parent->color = cstl_red;
                x = x->parent->parent;
            } else {
                if (x == x->parent->right) {
                    x = x->parent;
                    __left_rotate(pTree, x);
                }
                x->parent->color = cstl_black;
                x->parent->parent->color = cstl_red;
                __right_rotate(pTree, x->parent->parent);
            }
        } else {
            struct cstl_rb_node* y = x->parent->parent->left;
            if (y->color == cstl_red) {
                x->parent->color = cstl_black;
                y->color = cstl_black;
                x->parent->parent->color = cstl_red;
                x = x->parent->parent;
            } else {
                if (x == x->parent->left) {
                    x = x->parent;
                    __right_rotate(pTree, x);
                }
                x->parent->color = cstl_black;
                x->parent->parent->color = cstl_red;
                __left_rotate(pTree, x->parent->parent);
            }
        }
    }
    pTree->root->color = cstl_black;
}

struct cstl_rb_node*
cstl_rb_find(struct cstl_rb* pTree, void* key) {
    struct cstl_rb_node* x = pTree->root;

    while (x != rb_sentinel) {
        int c = 0;
        void* cur_key = (void *)0;
        cstl_object_get_raw(x->key, &cur_key);
        c = pTree->compare_fn(key, cur_key);
        free(cur_key);
        if (c == 0) {
            break;
        } else {
            x = c < 0 ? x->left : x->right;
        }
    }
    if (x == rb_sentinel) {
        return (struct cstl_rb_node*)0;
    }
    return x;
}

cstl_error
cstl_rb_insert(struct cstl_rb* pTree, void* k, size_t key_size, void* v, size_t value_size) {
    cstl_error rc = CSTL_ERROR_SUCCESS;
    struct cstl_rb_node* x;
    struct cstl_rb_node* y;
    struct cstl_rb_node* z;

    x = (struct cstl_rb_node*)calloc(1, sizeof(struct cstl_rb_node));
    if (x == (struct cstl_rb_node*)0) {
        return CSTL_ERROR_MEMORY;
    }
    x->left = rb_sentinel;
    x->right = rb_sentinel;
    x->color = cstl_red;

    x->key = cstl_object_new(k, key_size);
    if (v) {
        x->value = cstl_object_new(v, value_size);
    } else {
        x->value = (struct cstl_object*)0;
    }

    y = pTree->root;
    z = (struct cstl_rb_node*)0;

    while (y != rb_sentinel) {
        int c = 0;
        void* cur_key;
        void* new_key;

        cstl_object_get_raw(y->key, &cur_key);
        cstl_object_get_raw(x->key, &new_key);

        c = (pTree->compare_fn) (new_key, cur_key);
        free(cur_key);
        free(new_key);
        if (c == 0) {
            /* TODO : Delete node here */
            return CSTL_RBTREE_KEY_DUPLICATE;
        }
        z = y;
        if (c < 0) {
            y = y->left;
        } else {
            y = y->right;
        }
    }
    x->parent = z;
    if (z) {
        int c = 0;
        void* cur_key;
        void* new_key;
        cstl_object_get_raw(z->key, &cur_key);
        cstl_object_get_raw(x->key, &new_key);

        c = pTree->compare_fn(new_key, cur_key);
        free(cur_key);
        free(new_key);
        if (c < 0) {
            z->left = x;
        } else {
            z->right = x;
        }
    } else {
        pTree->root = x;
    }
    __rb_insert_fixup(pTree, x);

    debug_verify_properties(pTree);
    return rc;
}

static void
__rb_remove_fixup(struct cstl_rb* pTree, struct cstl_rb_node* x) {
    while (x != pTree->root && x->color == cstl_black) {
        if (x == x->parent->left) {
            struct cstl_rb_node* w = x->parent->right;
            if (w->color == cstl_red) {
                w->color = cstl_black;
                x->parent->color = cstl_red;
                __left_rotate(pTree, x->parent);
                w = x->parent->right;
            }
            if (w->left->color == cstl_black && w->right->color == cstl_black) {
                w->color = cstl_red;
                x = x->parent;
            } else {
                if (w->right->color == cstl_black) {
                    w->left->color = cstl_black;
                    w->color = cstl_red;
                    __right_rotate(pTree, w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = cstl_black;
                w->right->color = cstl_black;
                __left_rotate(pTree, x->parent);
                x = pTree->root;
            }
        } else {
            struct cstl_rb_node* w = x->parent->left;
            if (w->color == cstl_red) {
                w->color = cstl_black;
                x->parent->color = cstl_red;
                __right_rotate(pTree, x->parent);
                w = x->parent->left;
            }
            if (w->right->color == cstl_black && w->left->color == cstl_black) {
                w->color = cstl_red;
                x = x->parent;
            } else {
                if (w->left->color == cstl_black) {
                    w->right->color = cstl_black;
                    w->color = cstl_red;
                    __left_rotate(pTree, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = cstl_black;
                w->left->color = cstl_black;
                __right_rotate(pTree, x->parent);
                x = pTree->root;
            }
        }
    }
    x->color = cstl_black;
}

static struct cstl_rb_node*
__remove_c_rb(struct cstl_rb* pTree, struct cstl_rb_node* z) {
    struct cstl_rb_node* x = (struct cstl_rb_node*)0;
    struct cstl_rb_node* y = (struct cstl_rb_node*)0;

    if (z->left == rb_sentinel || z->right == rb_sentinel) {
        y = z;
    } else {
        y = z->right;
        while (y->left != rb_sentinel) {
            y = y->left;
        }
    }
    if (y->left != rb_sentinel) {
        x = y->left;
    } else {
        x = y->right;
    }
    x->parent = y->parent;
    if (y->parent) {
        if (y == y->parent->left) {
            y->parent->left = x;
        } else {
            y->parent->right = x;
        }
    } else {
        pTree->root = x;
    }
    if (y != z) {
        struct cstl_object* tmp;
        tmp = z->key;
        z->key = y->key;
        y->key = tmp;

        tmp = z->value;
        z->value = y->value;
        y->value = tmp;
    }
    if (y->color == cstl_black) {
        __rb_remove_fixup(pTree, x);
    }
    debug_verify_properties(pTree);
    return y;
}

struct cstl_rb_node*
cstl_rb_remove(struct cstl_rb* pTree, void* key) {
    struct cstl_rb_node* z = (struct cstl_rb_node*)0;

    z = pTree->root;
    while (z != rb_sentinel) {
        int c = 0;
        void* cur_key;
        cstl_object_get_raw(z->key, &cur_key);
        c = pTree->compare_fn(key, cur_key);
        free(cur_key);
        if (c == 0) {
            break;
        } else {
            z = (c < 0) ? z->left : z->right;
        }
    }
    if (z == rb_sentinel) {
        return (struct cstl_rb_node*)0;
    }
    return __remove_c_rb(pTree, z);
}

static void
__delete_c_rb_node(struct cstl_rb* pTree, struct cstl_rb_node* x) {
    void* key;
    void* value;

    if (pTree->destruct_k_fn) {
        cstl_object_get_raw(x->key, &key);
        pTree->destruct_k_fn(key);
        free(key);
    }
    cstl_object_delete(x->key);

    if (x->value) {
        if (pTree->destruct_v_fn) {
            cstl_object_get_raw(x->value, &value);
            pTree->destruct_v_fn(value);
            free(value);
        }
        cstl_object_delete(x->value);
    }
}

cstl_error
cstl_rb_delete(struct cstl_rb* pTree) {
    cstl_error rc = CSTL_ERROR_SUCCESS;
    struct cstl_rb_node* z = pTree->root;

    while (z != rb_sentinel) {
        if (z->left != rb_sentinel) {
            z = z->left;
        } else if (z->right != rb_sentinel) {
            z = z->right;
        } else {
            __delete_c_rb_node(pTree, z);
            if (z->parent) {
                z = z->parent;
                if (z->left != rb_sentinel) {
                    free(z->left);
                    z->left = rb_sentinel;
                } else if (z->right != rb_sentinel) {
                    free(z->right);
                    z->right = rb_sentinel;
                }
            } else {
                free(z);
                z = rb_sentinel;
            }
        }
    }
    free(pTree);
    return rc;
}

struct cstl_rb_node *
cstl_rb_minimum(struct cstl_rb* pTree, struct cstl_rb_node* x) {
    while (x->left != rb_sentinel) {
        x = x->left;
    }
    return x;
}

struct cstl_rb_node *
cstl_rb_maximum(struct cstl_rb* pTree, struct cstl_rb_node* x) {
    while (x->right != rb_sentinel) {
        x = x->right;
    }
    return x;
}

cstl_bool
cstl_rb_empty(struct cstl_rb* pTree) {
    if (pTree->root != rb_sentinel) {
        return cstl_true;
    }
    return cstl_false;
}

struct cstl_rb_node*
cstl_rb_tree_successor(struct cstl_rb* pTree, struct cstl_rb_node* x) {
    struct cstl_rb_node *y = (struct cstl_rb_node*)0;
    if (x->right != rb_sentinel) {
        return cstl_rb_minimum(pTree, x->right);
    }
    if (x == cstl_rb_maximum(pTree, pTree->root)) {
        return (struct cstl_rb_node*)0;
    }
    y = x->parent;
    while (y != rb_sentinel && x == y->right) {
        x = y;
        y = y->parent;
    }
    return y;
}

/*
struct cstl_rb_node *
cstl_rb_get_next(struct cstl_rb* pTree, struct cstl_rb_node**current, struct cstl_rb_node**pre) {
    struct cstl_rb_node* prev_current;
    while ((*current) != rb_sentinel) {
        if ((*current)->left == rb_sentinel) {
            prev_current = (*current);
            (*current) = (*current)->right;
            return prev_current->raw_data.key;
        } else {
            (*pre) = (*current)->left;
            while ((*pre)->right != rb_sentinel && (*pre)->right != (*current))
                (*pre) = (*pre)->right;
            if ((*pre)->right == rb_sentinel) {
                (*pre)->right = (*current);
                (*current) = (*current)->left;
            } else {
                (*pre)->right = rb_sentinel;
                prev_current = (*current);
                (*current) = (*current)->right;
                return prev_current->raw_data.key;
            }
        }
    }
    return (struct cstl_rb_node*)0;
} */

void debug_verify_properties(struct cstl_rb* t) {
    debug_verify_property_1(t, t->root);
    debug_verify_property_2(t, t->root);
    debug_verify_property_4(t, t->root);
    debug_verify_property_5(t, t->root);
}

void debug_verify_property_1(struct cstl_rb* pTree, struct cstl_rb_node* n) {
    assert(debug_node_color(pTree, n) == cstl_red || debug_node_color(pTree, n) == cstl_black);
    if (n == rb_sentinel) { return; }
    debug_verify_property_1(pTree, n->left);
    debug_verify_property_1(pTree, n->right);
}

void debug_verify_property_2(struct cstl_rb* pTree, struct cstl_rb_node* root) {
    assert(debug_node_color(pTree, root) == cstl_black);
}

int debug_node_color(struct cstl_rb* pTree, struct cstl_rb_node* n) {
    return n == rb_sentinel ? cstl_black : n->color;
}

void debug_verify_property_4(struct cstl_rb* pTree, struct cstl_rb_node* n) {
    if (debug_node_color(pTree, n) == cstl_red) {
        assert(debug_node_color(pTree, n->left) == cstl_black);
        assert(debug_node_color(pTree, n->right) == cstl_black);
        assert(debug_node_color(pTree, n->parent) == cstl_black);
    }
    if (n == rb_sentinel) { return; }
    debug_verify_property_4(pTree, n->left);
    debug_verify_property_4(pTree, n->right);
}

void debug_verify_property_5(struct cstl_rb* pTree, struct cstl_rb_node* root) {
    int black_count_path = -1;
    debug_verify_property_5_helper(pTree, root, 0, &black_count_path);
}

void debug_verify_property_5_helper(struct cstl_rb* pTree, struct cstl_rb_node* n, int black_count, int* path_black_count) {
    if (debug_node_color(pTree, n) == cstl_black) {
        black_count++;
    }
    if (n == rb_sentinel) {
        if (*path_black_count == -1) {
            *path_black_count = black_count;
        } else {
            assert(black_count == *path_black_count);
        }
        return;
    }
    debug_verify_property_5_helper(pTree, n->left, black_count, path_black_count);
    debug_verify_property_5_helper(pTree, n->right, black_count, path_black_count);
}
