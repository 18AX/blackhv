#include <blackhv/linked_list.h>
#include <stdlib.h>

linked_list_t *linked_list_new(void (*free_func)(void *))
{
    linked_list_t *list = malloc(sizeof(linked_list_t));

    if (list == NULL)
    {
        return NULL;
    }

    list->free_func = free_func;
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;

    return list;
}

unsigned int linked_list_add(linked_list_t *list, void *value)
{
    if (list == NULL)
    {
        return 0;
    }

    struct linked_list_elt *elt = malloc(sizeof(struct linked_list_elt));

    if (elt == NULL)
    {
        return 0;
    }

    elt->value = value;
    elt->prev = list->tail;
    elt->next = NULL;

    if (list->tail != NULL)
    {
        list->tail->next = elt;
        list->head->prev = elt;
    }
    else
    {
        list->head = elt;
    }

    list->tail = elt;
    ++list->size;

    return 1;
}

void *linked_list_get(linked_list_t *list, unsigned int index)
{
    if (list == NULL)
    {
        return NULL;
    }

    struct linked_list_elt *elt = list->head;

    for (unsigned int i = 0; elt != NULL; ++i)
    {
        if (i == index)
        {
            return elt->value;
        }

        elt = elt->next;
    }

    return NULL;
}

void *linked_list_pop_tail(linked_list_t *list)
{
    if (list == NULL || list->tail == NULL)
    {
        return NULL;
    }

    struct linked_list_elt *tail = list->tail;

    void *value = tail->value;

    if (tail == list->head)
    {
        list->tail = NULL;
        list->head = NULL;
        list->size = 0;
    }
    else
    {
        tail->prev->next = NULL;
        --list->size;
    }

    list->tail = tail->prev;

    free(tail);

    return value;
}

void *linked_list_pop_front(linked_list_t *list)
{
    if (list == NULL || list->head == NULL)
    {
        return NULL;
    }

    struct linked_list_elt *head = list->head;

    void *value = head->value;

    if (head == list->tail)
    {
        list->head = NULL;
        list->tail = NULL;
        list->size = 0;
    }
    else
    {
        head->next->prev = NULL;
        --list->size;
    }

    list->head = head->next;

    free(head);

    return value;
}

static void remove_elt(linked_list_t *list, struct linked_list_elt *elt)
{
    if (elt->prev != NULL)
    {
        elt->prev->next = elt->next;
    }

    if (elt->next != NULL)
    {
        elt->next->prev = elt->prev;
    }

    if (list->head == elt)
    {
        list->head = elt->next;
    }

    if (list->tail == elt)
    {
        list->tail = elt->prev;
    }

    if (list->free_func != NULL)
    {
        list->free_func(elt->value);
    }

    free(elt);

    --list->size;
}

unsigned int linked_list_remove_at(linked_list_t *list, unsigned int index)
{
    if (list == NULL)
    {
        return 0;
    }

    struct linked_list_elt *elt = list->head;

    for (unsigned int i = 0; elt != NULL; ++i)
    {
        if (i == index)
        {
            remove_elt(list, elt);
            return 1;
        }

        elt = elt->next;
    }

    return 0;
}

unsigned int linked_list_remove_eq(linked_list_t *list,
                                   unsigned int (*eq_func)(void *))
{
    if (list == NULL || eq_func == NULL)
    {
        return 0;
    }

    unsigned int removed = 0;

    struct linked_list_elt *current = list->head;

    while (current != NULL)
    {
        struct linked_list_elt *tmp = current->next;

        if (eq_func(current->value) == 1)
        {
            remove_elt(list, current);
            ++removed;
        }

        current = tmp;
    }

    return removed;
}

void linked_list_for_each(linked_list_t *list,
                          void (*func)(unsigned int, void *value))
{
    if (list == NULL || func == NULL)
    {
        return;
    }

    struct linked_list_elt *current = list->head;

    for (unsigned int i = 0; current != NULL; ++i)
    {
        func(i, current->value);
        current = current->next;
    }
}

void linked_list_for_each_rev(linked_list_t *list,
                              void (*func)(unsigned int, void *vale))
{
    if (list == NULL || func == NULL)
    {
        return;
    }

    struct linked_list_elt *current = list->tail;

    for (unsigned int i = list->size - 1; current != NULL; --i)
    {
        func(i, current->value);
        current = current->prev;
    }
}

unsigned long linked_list_size(linked_list_t *list)
{
    if (list == NULL)
    {
        return 0;
    }

    return list->size;
}

void linked_list_free(linked_list_t *list)
{
    if (list == NULL)
    {
        return;
    }

    // Free all the list elements
    struct linked_list_elt *current = list->head;

    while (current != NULL)
    {
        struct linked_list_elt *tmp = current->next;

        if (list->free_func != NULL)
        {
            list->free_func(current->value);
        }

        free(current);
        current = tmp;
    }

    free(list);
}
