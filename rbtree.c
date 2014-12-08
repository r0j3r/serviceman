#include "rbtree.h"

int is_red(struct node * n) {
    return n->flags == 1;
}

int jsw_remove ( struct tree *tree, int data ) {
   if ( tree->root != 0) {
     struct node head = {0}; /* False tree root */
     struct node *q, *p, *g; /* Helpers */
     struct node *f = 0;  /* Found item */
     int dir = 1;
 
     /* Set up helpers */
     q = &head;
     g = p = 0;
     q->c[1] = tree->root;
 
     /* Search and push a red down */
     while ( q->c[dir] != 0 ) {
       int last = dir;
 
       /* Update helpers */
       g = p, p = q;
       q = q->c[dir];
       dir = q->data < data;
 
       /* Save found node */
       if ( q->data == data )
         f = q;
 
       /* Push the red node down */
       if ( !is_red ( q ) && !is_red ( q->c[dir] ) ) {
         if ( is_red ( q->c[!dir] ) )
           p = p->c[last] = jsw_single ( q, dir );
         else if ( !is_red ( q->c[!dir] ) ) {
           struct node *s = p->c[!last];
 
           if ( s != 0 ) {
             if ( !is_red ( s->c[!last] ) && !is_red ( s->c[last] ) ) {
               /* Color flip */
               p->flags = 0;
               s->flags = 1;
               q->flags = 1;
             }
             else {
               int dir2 = g->c[1] == p;
 
               if ( is_red ( s->c[last] ) )
                 g->c[dir2] = jsw_double ( p, last );
               else if ( is_red ( s->c[!last] ) )
                 g->c[dir2] = jsw_single ( p, last );
 
               /* Ensure correct coloring */
               q->flags = g->c[dir2]->flags = 1;
               g->c[dir2]->c[0]->flags = 0;
               g->c[dir2]->c[1]->flags = 0;
             }
           }
         }
       }
     }
 
     /* Replace and remove if found */
     if ( f != 0 ) {
       f->data = q->data;
       p->c[p->c[1] == q] =
         q->c[q->c[0] == 0];
       free ( q );
     }
 
     /* Update root and make it black */
     tree->root = head.c[1];
     if ( tree->root != 0 )
       tree->root->flags = 0;
   }
 
   return 1;
 }

int jsw_insert ( struct tree *tree, int data )
{
   if ( tree->root == 0 ) {
     /* Empty tree case */
    tree->root = make_node ( data );
    if ( tree->root == 0 )
       return 0;
   }
   else {
     struct node head = {0}; /* False tree root */
 
     struct node *g, *t;     /* Grandparent & parent */
     struct node *p, *q;     /* Iterator & parent */
     int dir = 0, last;
 
     /* Set up helpers */
     t = &head;
     g = p = 0;
     q = t->c[1] = tree->root;
 
     /* Search down the tree */
     for ( ; ; ) {
       if ( q == 0 ) {
         /* Insert new node at the bottom */
         p->c[dir] = q = make_node ( data );
         if ( q == 0 )
           return 0;
       }
       else if ( is_red ( q->c[0] ) && is_red ( q->c[1] ) ) {
         /* Color flip */
         q->flags = 1;
         q->c[0]->flags = 0;
         q->c[1]->flags = 0;
       }
 
       /* Fix red violation */
       if ( is_red ( q ) && is_red ( p ) ) {
         int dir2 = t->c[1] == g;
 
         if ( q == p->c[last] )
           t->c[dir2] = jsw_single ( g, !last );
         else
           t->c[dir2] = jsw_double ( g, !last );
       }
 
      /* Stop if found */
       if ( q->data == data )
         break;
 
       last = dir;
       dir = q->data < data;
 
       /* Update helpers */
       if ( g != 0 )
         t = g;
       g = p, p = q;
       q = q->c[dir];
     }
 
     /* Update root */
     tree->root = head.c[1];
   }

   /* Make root black */
   tree->root->flags = 0;
 
   return 1;
}
