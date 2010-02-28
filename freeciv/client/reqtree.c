/********************************************************************** 
 Freeciv - Copyright (C) 2005-2007 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <log.h>
#include <stdarg.h>
#include <string.h>

/* common */
#include "government.h"
#include "improvement.h"
#include "tech.h"

/* client */
#include "client_main.h"
#include "tilespec.h"

#include "colors_g.h"
#include "sprite_g.h"

#include "reqtree.h"
#include "tilespec.h"
#include "options.h"

/*
 * Hierarchical directed draph drawing for Freeciv's technology tree
 *
 *
 * \     Layer 0    /          \    Layer 1    /   \  Layer 2  /
 *  vvvvvvvvvvvvvvvv            vvvvvvvvvvvvvvv     vvvvvvvvvvv
 *
 * +-----------------+          +-------------+    +----------+
 * |    Alphabeth    |----------| Code of Laws|----| Monarchy |
 * +-----------------+          +-------------+   /+----------+
 *                                               /
 * +-----------------+             Dummy node  / 
 * |Ceremonial Burial|-----------=============/
 * +-----------------+
 * 
 * ^ node_y
 * |
 * |
 * |    node_x
 * +-------->
 */



/****************************************************************************
  This structure desribes a node in a technology tree diagram.
  A node can by dummy or real. Real node describes a technology.
****************************************************************************/
struct tree_node {
  bool is_dummy;
  Tech_type_id tech;

  /* Incoming edges */
  int nrequire;
  struct tree_node **require;

  /* Outgoing edges */
  int nprovide;
  struct tree_node **provide;

  /* logical position on the diagram */
  int order, layer;

  /* Coordinates of the rectangle on the diagram in pixels */
  int node_x, node_y, node_width, node_height;

  /* for general purpose */
  int number;
};

/****************************************************************************
  Structure which describes abstract technology diagram.
  Nodes are ordered inside layers[] table.
****************************************************************************/
struct reqtree {
  int num_nodes;
  struct tree_node **nodes;

  int num_layers;
  /* size of each layer */
  int *layer_size;
  struct tree_node ***layers;

  /* in pixels */
  int diagram_width, diagram_height;
};


/****************************************************************************
  Edge types for coloring the edges by type in the tree
****************************************************************************/
enum reqtree_edge_type {
  REQTREE_EDGE = 0,     /* Normal, "unvisited" */
  REQTREE_READY_EDGE,
  REQTREE_KNOWN_EDGE,   /* Both nodes known, "visited" */
  REQTREE_ACTIVE_EDGE,
  REQTREE_GOAL_EDGE     /* Dest node is part of goal "future visited" */
};

/*************************************************************************
  Add requirement edge to node and provide edge to req
*************************************************************************/
static void add_requirement(struct tree_node *node, struct tree_node *req)
{
  node->require =
      fc_realloc(node->require,
		 sizeof(*node->require) * (node->nrequire + 1));
  node->require[node->nrequire] = req;
  node->nrequire++;

  req->provide =
      fc_realloc(req->provide,
		 sizeof(*req->provide) * (req->nprovide + 1));
  req->provide[req->nprovide] = node;
  req->nprovide++;
}

/*************************************************************************
  Allocate and initialize new tree node
*************************************************************************/
static struct tree_node *new_tree_node(void)
{
  struct tree_node *node = fc_malloc(sizeof(*node));

  node->nrequire = 0;
  node->nprovide = 0;
  node->require = NULL;
  node->provide = NULL;
  node->order = -1;
  node->layer = -1;
  return node;
}

/****************************************************************************
  Return minimum size of the rectangle in pixels on the diagram which
  corresponds to the given node
****************************************************************************/
static void node_rectangle_minimum_size(struct tree_node *node,
					int *width, int *height)
{
  int max_icon_height; /* maximal height of icons below the text */
  int icons_width_sum; /* sum of icons width plus space between them */
  struct sprite* sprite;
  int swidth, sheight;
  
  if (node->is_dummy) {
    /* Dummy node is a straight line */
    *width = *height = 1;
  } else {
    get_text_size(width, height, FONT_REQTREE_TEXT,
		  advance_name_for_player(client.conn.playing, node->tech));
    *width += 2;
    *height += 8;
    
    max_icon_height = 0;
    icons_width_sum = 5;
    
    if (reqtree_show_icons) {
      /* units */
      unit_type_iterate(unit) {
        if (advance_number(unit->require_advance) != node->tech) {
          continue;
        }
        sprite = get_unittype_sprite(tileset, unit);
        get_sprite_dimensions(sprite, &swidth, &sheight);
        max_icon_height = MAX(max_icon_height, sheight);
        icons_width_sum += swidth + 2;
      } unit_type_iterate_end;
    
      /* buildings */
      improvement_iterate(pimprove) {
        requirement_vector_iterate(&(pimprove->reqs), preq) {
          if (VUT_ADVANCE == preq->source.kind
	   && advance_number(preq->source.value.advance) == node->tech) {
	    sprite = get_building_sprite(tileset, pimprove);
            /* Improvement icons are not guaranteed to exist */
            if (sprite) {
              get_sprite_dimensions(sprite, &swidth, &sheight);
              max_icon_height = MAX(max_icon_height, sheight);
              icons_width_sum += swidth + 2;
            }
	  }
        } requirement_vector_iterate_end;
      } improvement_iterate_end;
    
      /* governments */
      government_iterate(gov) {
        requirement_vector_iterate(&(gov->reqs), preq) {
          if (VUT_ADVANCE == preq->source.kind
	   && advance_number(preq->source.value.advance) == node->tech) {
            sprite = get_government_sprite(tileset, gov);
	    get_sprite_dimensions(sprite, &swidth, &sheight);
            max_icon_height = MAX(max_icon_height, sheight);
            icons_width_sum += swidth + 2;	    
	  }
        } requirement_vector_iterate_end;
      } government_iterate_end;
    }
    
    *height += max_icon_height;
    if (*width < icons_width_sum) {
      *width = icons_width_sum;
    }
  }
}

/****************************************************************************
  Move nodes up and down without changing order but making it more 
  symetrical. Gravitate towards parents average position.
****************************************************************************/
static void symmetrize(struct reqtree* tree)
{
  int layer;
  int i, j;
  
  for (layer = 0; layer < tree->num_layers; layer++) {
    for (i = 0; i < tree->layer_size[layer]; i++) {
      struct tree_node *node = tree->layers[layer][i];
      int v, node_y, node_height;

      if (node->nrequire == 0) {
        continue;
      }
      v = 0;
      for (j = 0; j < node->nrequire; j++) {
        struct tree_node *node_require = node->require[j];

        v += node_require->node_y + node_require->node_height / 2;
      }
      v /= node->nrequire;
      node_y = node->node_y;
      node_height = node->node_height;
      if (v < node_y + node_height / 2) {
        if (node_y <= 0) {
	  continue;
	}
	if (i > 0) {
	  struct tree_node *node_above = tree->layers[layer][i - 1];

	  if (node_above->node_y
	      + node_above->node_height >= node_y - 11) {
	    continue;
	  }
	}
	node_y--;
      } else if (v > node_y + node_height / 2) {
        if (node_y + node_height >= tree->diagram_height - 1) {
	  continue;
	}
	if (i < tree->layer_size[layer] - 1) {
	  struct tree_node* node_below = tree->layers[layer][i + 1];

	  if (node_y + node_height >= node_below->node_y - 11) {
	    continue;
	  }
	}
	node_y++;
      }
      node->node_y = node_y;
    }
  }
}

/****************************************************************************
  Calculate rectangles position and size from the tree.
  Logical order should already be calculated.
****************************************************************************/
static void calculate_diagram_layout(struct reqtree *tree)
{
  int i, layer, layer_offs;

  /* calculate minimum size of rectangle for each node */
  for (i = 0; i < tree->num_nodes; i++) {
    struct tree_node *node = tree->nodes[i];

    node_rectangle_minimum_size(tree->nodes[i],
				&node->node_width, &node->node_height);
    node->number = i;
  }
  
  /* calculate height of the diagram. There should be at least 10 pixels
   * beetween any two nodes */
  tree->diagram_height = 0;
  for (layer = 0; layer < tree->num_layers; layer++) {
    int h_sum = 0;

    for (i = 0; i < tree->layer_size[layer]; i++) {
      struct tree_node *node = tree->layers[layer][i];

      h_sum += node->node_height;
      if (i < tree->layer_size[layer] - 1) {
	h_sum += 10;
      }
    }
    tree->diagram_height = MAX(tree->diagram_height, h_sum);
  }
  
  /* calculate maximum width of node for each layer and enlarge other nodes
   * to match maximum width
   * calculate x offsets
   */
  layer_offs = 0;
  for (layer = 0; layer < tree->num_layers; layer++) {
    int max_width = 0;

    for (i = 0; i < tree->layer_size[layer]; i++) {
      struct tree_node *node = tree->layers[layer][i];

      max_width = MAX(max_width, node->node_width);
    }
    
    for (i = 0; i < tree->layer_size[layer]; i++) {
      struct tree_node *node = tree->layers[layer][i];

      node->node_width = max_width;
      node->node_x = layer_offs;
    }
    
    /* space between layers should be proportional to their size */
    if (layer != tree->num_layers - 1)  {
      layer_offs += max_width * 5 / 4 + 80;
    } else {
      layer_offs += max_width + 10;
    }
  }
  tree->diagram_width = layer_offs;

  /* Once we have x positions calculated we can
   * calculate y-position of nodes on the diagram 
   * Distribute nodes steadily.
   */
  for (layer = 0; layer < tree->num_layers; layer++) {
    int y = 0;
    int h_sum = 0;

    for (i = 0; i < tree->layer_size[layer]; i++) {
      struct tree_node *node = tree->layers[layer][i];

      h_sum += node->node_height;
    }
    for (i = 0; i < tree->layer_size[layer]; i++) {
      struct tree_node *node = tree->layers[layer][i];

      node->node_y = y;
      y += node->node_height;
      if (tree->layer_size[layer] > 1) {
	y += (tree->diagram_height - h_sum)
	  / (tree->layer_size[layer] - 1) - 1;
      }
    }
  }

  /* The symetrize() function moves node by one pixel per call */
  for (i = 0; i < tree->diagram_height; i++) {
    symmetrize(tree);
  }
}


/*************************************************************************
  Create a "dummy" tech tree from current ruleset.  This tree is then
  fleshed out further (see create_reqtree). This tree doesn't include
  dummy edges. Layering and ordering isn't done also.

  If pplayer is given, add only techs reachable by that player to tree.
*************************************************************************/
static struct reqtree *create_dummy_reqtree(struct player *pplayer)
{
  struct reqtree *tree = fc_malloc(sizeof(*tree));
  int j;
  struct tree_node *nodes[advance_count()];

  nodes[A_NONE] = NULL;
  advance_index_iterate(A_FIRST, tech) {
    if (!valid_advance_by_number(tech)) {
      nodes[tech] = NULL;
      continue;
    }
    if (pplayer && !player_invention_reachable(pplayer, tech)) {
      /* Reqtree requested for particular player and this tech is
       * unreachable to him/her. */
      nodes[tech] = NULL;
      continue;
    }
    nodes[tech] = new_tree_node();
    nodes[tech]->is_dummy = FALSE;
    nodes[tech]->tech = tech;
  } advance_index_iterate_end;

  advance_index_iterate(A_FIRST, tech) {
    struct advance *padvance = valid_advance_by_number(tech);

    if (!padvance) {
      continue;
    }
    if (nodes[tech] == NULL) {
      continue;
    }

    /* Don't include redundant edges */
    if (A_NONE != advance_required(tech, AR_ONE)
     && A_LAST != advance_required(tech, AR_TWO)) {
      if (A_NONE == advance_required(tech, AR_TWO)
       || !is_tech_a_req_for_goal(client.conn.playing,
				  advance_required(tech, AR_ONE),
				  advance_required(tech, AR_TWO))) {
	add_requirement(nodes[tech], nodes[advance_required(tech, AR_ONE)]);
      }

      if (A_NONE != advance_required(tech, AR_TWO)
       && !is_tech_a_req_for_goal(client.conn.playing,
				  advance_required(tech, AR_TWO),
				  advance_required(tech, AR_ONE))) {
	add_requirement(nodes[tech], nodes[advance_required(tech, AR_TWO)]);
      }
    }
  } advance_index_iterate_end;

  /* Copy nodes from local array to dynamically allocated one. 
   * Skip non-existing entries */
  tree->nodes = fc_calloc(advance_count(), sizeof(*tree->nodes));
  j = 0;
  advance_index_iterate(A_FIRST, tech) {
    if (nodes[tech]) {
      assert(valid_advance_by_number(nodes[tech]->tech));
      tree->nodes[j++] = nodes[tech];
    }
  } advance_index_iterate_end;
  tree->num_nodes = j;
  tree->layers = NULL;

  return tree;
}

/*************************************************************************
  Free all memory used by tech_tree struct
*************************************************************************/
void destroy_reqtree(struct reqtree *tree)
{
  int i;

  for (i = 0; i < tree->num_nodes; i++) {
    free(tree->nodes[i]->require);
    free(tree->nodes[i]->provide);
    free(tree->nodes[i]);
  }
  free(tree->nodes);
  if (tree->layers) {
    for (i = 0; i < tree->num_layers; i++) {
      free(tree->layers[i]);
    }
    if (tree->layer_size) {
      free(tree->layer_size);
    }
  }
  free(tree);
}

/*************************************************************************
  Compute the longest path from this tree_node to the node with 
  no requirements. Store the result in node->layer.
*************************************************************************/
static int longest_path(struct tree_node *node)
{
  int max, i;

  if (node->layer != -1) {
    return node->layer;
  }
  max = -1;
  for (i = 0; i < node->nrequire; i++) {
    max = MAX(max, longest_path(node->require[i]));
  }
  node->layer = max + 1;
  return node->layer;
}

/*************************************************************************
  Compute longest_path for all nodes, thus prepare longest path layering
*************************************************************************/
static void longest_path_layering(struct reqtree *tree)
{
  int i;

  for (i = 0; i < tree->num_nodes; i++) {
    if (tree->nodes[i]) {
      longest_path(tree->nodes[i]);
    }
  }
}

/*************************************************************************
  Find the largest value of layer amongst children of the given node
*************************************************************************/
static int max_provide_layer(struct tree_node *node)
{
  int i;
  int max = node->layer;

  for (i = 0; i < node->nprovide; i++) {
    if (node->provide[i]->layer > max) {
      max = node->provide[i]->layer;
    }
  }
  return max;
}

/*************************************************************************
  Create new tree which has dummy nodes added. The source tree is 
  completely copied, you can freely deallocate it.
*************************************************************************/
static struct reqtree *add_dummy_nodes(struct reqtree *tree)
{
  struct reqtree *new_tree;
  int num_dummy_nodes = 0;
  int k, i, j;

  /* Count dummy nodes to be added */
  for (i = 0; i < tree->num_nodes; i++) {
    int mpl;

    if (tree->nodes[i] == NULL) {
      continue;
    }
    mpl = max_provide_layer(tree->nodes[i]);
    if (mpl > tree->nodes[i]->layer + 1) {
      num_dummy_nodes += mpl - tree->nodes[i]->layer - 1;
    }
  }

  /* create new tree */
  new_tree = fc_malloc(sizeof(*new_tree));
  new_tree->nodes =
      fc_malloc(sizeof(new_tree->nodes) *
		(tree->num_nodes + num_dummy_nodes));
  new_tree->num_nodes = tree->num_nodes + num_dummy_nodes;
  
  /* copy normal nodes */
  for (i = 0; i < tree->num_nodes; i++) {
    new_tree->nodes[i] = new_tree_node();
    new_tree->nodes[i]->is_dummy = FALSE;
    new_tree->nodes[i]->tech = tree->nodes[i]->tech;
    new_tree->nodes[i]->layer = tree->nodes[i]->layer;
    tree->nodes[i]->number = i;
  }
  
  /* allocate dummy nodes */
  for (i = 0; i < num_dummy_nodes; i++) {
    new_tree->nodes[i + tree->num_nodes] = new_tree_node();
    new_tree->nodes[i + tree->num_nodes]->is_dummy = TRUE;
  }
  /* k points to the first unused dummy node */
  k = tree->num_nodes;

  for (i = 0; i < tree->num_nodes; i++) {
    struct tree_node *node = tree->nodes[i];
    int mpl;

    assert(!node->is_dummy);

    mpl = max_provide_layer(node);

    /* if this node will have dummy as ancestors, connect them in a chain */
    if (mpl > node->layer + 1) {
      add_requirement(new_tree->nodes[k], new_tree->nodes[i]);
      for (j = node->layer + 2; j < mpl; j++) {
	add_requirement(new_tree->nodes[k + j - node->layer - 1],
			new_tree->nodes[k + j - node->layer - 2]);
      }
      for (j = node->layer + 1; j < mpl; j++) {
	new_tree->nodes[k + j - node->layer - 1]->layer = j;
      }
    }

    /* copy all edges and create edges with dummy nodes */
    for (j = 0; j < node->nprovide; j++) {
      int provide_y = node->provide[j]->layer;

      if (provide_y == node->layer + 1) {
        /* direct connection */
	add_requirement(new_tree->nodes[node->provide[j]->number],
			new_tree->nodes[i]);
      } else {
        /* connection through dummy node */
	add_requirement(new_tree->nodes[node->provide[j]->number],
			new_tree->nodes[k + provide_y - node->layer - 2]);
      }
    }

    if (mpl > node->layer + 1) {
      k += mpl - node->layer - 1;
      assert(k <= new_tree->num_nodes);
    }
  }
  new_tree->layers = NULL;

  return new_tree;
}

/*************************************************************************
  Calculate layers[] and layer_size[] fields of tree.
  There should be layer value calculated for each node.
  Nodes will be put into layers in no particular order.
*************************************************************************/
static void set_layers(struct reqtree *tree)
{
  int i;
  int num_layers = 0;
  
  /* count total number of layers */
  for (i = 0; i < tree->num_nodes; i++) {
    num_layers = MAX(num_layers, tree->nodes[i]->layer);
  }
  num_layers++;
  tree->num_layers = num_layers;

  {
    /* Counters for order - order number for the next node in the layer */
    int T[num_layers];

    tree->layers = fc_malloc(sizeof(*tree->layers) * num_layers);
    tree->layer_size = fc_malloc(sizeof(*tree->layer_size) * num_layers);
    for (i = 0; i < num_layers; i++) {
      T[i] = 0;
      tree->layer_size[i] = 0;
    }
    for (i = 0; i < tree->num_nodes; i++) {
      tree->layer_size[tree->nodes[i]->layer]++;
    }

    for (i = 0; i < num_layers; i++) {
      tree->layers[i] =
	  fc_malloc(sizeof(*tree->layers[i]) * tree->layer_size[i]);
    }
    for (i = 0; i < tree->num_nodes; i++) {
      struct tree_node *node = tree->nodes[i];

      tree->layers[node->layer][T[node->layer]] = node;
      node->order = T[node->layer];
      T[node->layer]++;
    }
  }
}

struct node_and_float {
  struct tree_node *node;
  float value;
};

/*************************************************************************
  Comparison function used by barycentric_sort.
*************************************************************************/
static int cmp_func(const void *_a, const void *_b)
{
  const struct node_and_float *a = _a, *b = _b;

  if (a->value > b->value) {
    return 1;
  }
  if (a->value < b->value) {
    return -1;
  }
  return 0;
}

/*************************************************************************
  Simple heuristic: Sort nodes on the given layer by the average x-value
  of its' parents.
*************************************************************************/
static void barycentric_sort(struct reqtree *tree, int layer)
{
  struct node_and_float T[tree->layer_size[layer]];
  int i, j;
  float v;

  for (i = 0; i < tree->layer_size[layer]; i++) {
    struct tree_node *node = tree->layers[layer][i];

    T[i].node = node;
    v = 0.0;
    for (j = 0; j < node->nrequire; j++) {
      v += node->require[j]->order;
    }
    if (node->nrequire > 0) {
      v /= (float) node->nrequire;
    }
    T[i].value = v;
  }
  qsort(T, tree->layer_size[layer], sizeof(*T),
	cmp_func);

  for (i = 0; i < tree->layer_size[layer]; i++) {
    tree->layers[layer][i] = T[i].node;
    T[i].node->order = i;
  }
}

/*************************************************************************
  Calculate number of edge crossings beetwen layer and layer+1
*************************************************************************/
static int count_crossings(struct reqtree *tree, int layer)
{
  int layer1_size = tree->layer_size[layer];
  int layer2_size = tree->layer_size[layer + 1];
  int X[layer2_size];
  int i, j, k;
  int sum = 0;

  for (i = 0; i < layer2_size; i++) {
    X[i] = 0;
  }

  for (i = 0; i < layer1_size; i++) {
    struct tree_node *node = tree->layers[layer][i];

    for (j = 0; j < node->nprovide; j++) {
      sum += X[node->provide[j]->order];
    }
    for (j = 0; j < node->nprovide; j++) {
      for (k = 0; k < node->provide[j]->order; k++) {
	X[k]++;
      }
    }
  }

  return sum;
}

/*************************************************************************
  Swap positions of two nodes on the same layer
*************************************************************************/
static void swap(struct reqtree *tree, int layer, int order1, int order2)
{
  struct tree_node *node1 = tree->layers[layer][order1];
  struct tree_node *node2 = tree->layers[layer][order2];

  tree->layers[layer][order1] = node2;
  tree->layers[layer][order2] = node1;
  node1->order = order2;
  node2->order = order1;
}

/*************************************************************************
  Try to reduce the number of crossings by swapping two nodes and checking
  if it improves the situation.
*************************************************************************/
static void improve(struct reqtree *tree)
{
  int crossings[tree->num_layers - 1];
  int i, x1, x2, layer;

  for (i = 0; i < tree->num_layers - 1; i++) {
    crossings[i] = count_crossings(tree, i);
  }

  for (layer = 0; layer < tree->num_layers; layer++) {
    int layer_size = tree->layer_size[layer];
    int layer_sum = 0;

    if (layer > 0) {
      layer_sum += crossings[layer - 1];
    }
    if (layer < tree->num_layers - 1) {
      layer_sum += crossings[layer];
    }

    for (x1 = 0; x1 < layer_size; x1++) {
      for (x2 = x1 + 1; x2 < layer_size; x2++) {
	int new_crossings = 0;
	int new_crossings_before = 0;

	swap(tree, layer, x1, x2);
	if (layer > 0) {
	  new_crossings_before += count_crossings(tree, layer - 1);
	}
	if (layer < tree->num_layers - 1) {
	  new_crossings += count_crossings(tree, layer);
	}
	if (new_crossings + new_crossings_before > layer_sum) {
	  swap(tree, layer, x1, x2);
	} else {
	  layer_sum = new_crossings + new_crossings_before;
	  if (layer > 0) {
	    crossings[layer - 1] = new_crossings_before;
	  }
	  if (layer < tree->num_layers - 1) {
	    crossings[layer] = new_crossings;
	  }
	}
      }
    }
  }
}

/*************************************************************************
  Generate optimized tech_tree from current ruleset.
  You should free it by destroy_reqtree.

  If pplayer is not NULL, techs unreachable to that player are not shown.
*************************************************************************/
struct reqtree *create_reqtree(struct player *pplayer)
{
  struct reqtree *tree1, *tree2;
  int i, j;

  tree1 = create_dummy_reqtree(pplayer);
  longest_path_layering(tree1);
  tree2 = add_dummy_nodes(tree1);
  destroy_reqtree(tree1);
  set_layers(tree2);
  
  /* It's good heuristics for beginning */
  for (j = 0; j < 20; j++) {
    for (i = 0; i < tree2->num_layers; i++) {
      barycentric_sort(tree2, i);
    }
  }
  
  /* Now burn some CPU */
  for (j = 0; j < 20; j++) {
    improve(tree2);
  }

  calculate_diagram_layout(tree2);

  return tree2;
}

/****************************************************************************
  Give the dimensions of the reqtree.
****************************************************************************/
void get_reqtree_dimensions(struct reqtree *reqtree,
			    int *width, int *height)
{
  if (width) {
    *width = reqtree->diagram_width;
  }
  if (height){
    *height = reqtree->diagram_height;
  }
}

/****************************************************************************
  Return a background color of node's rectangle
****************************************************************************/
static enum color_std node_color(struct tree_node *node)
{
  if (!node->is_dummy) {
    struct player_research* research = get_player_research(client.conn.playing);

    if (!research) {
      return COLOR_REQTREE_KNOWN;
    }

    if (!player_invention_reachable(client.conn.playing, node->tech)) {
      return COLOR_REQTREE_UNREACHABLE;
    }

    if (research->researching == node->tech) {
      return COLOR_REQTREE_RESEARCHING;
    }
    
    if (TECH_KNOWN == player_invention_state(client.conn.playing, node->tech)) {
      return COLOR_REQTREE_KNOWN;
    }

    if (is_tech_a_req_for_goal(client.conn.playing, node->tech,
                               research->tech_goal)
	|| node->tech == research->tech_goal) {
      if (TECH_PREREQS_KNOWN ==
            player_invention_state(client.conn.playing, node->tech)) {
	return COLOR_REQTREE_GOAL_PREREQS_KNOWN;
      } else {
	return COLOR_REQTREE_GOAL_UNKNOWN;
      }
    }

    if (TECH_PREREQS_KNOWN ==
          player_invention_state(client.conn.playing, node->tech)) {
      return COLOR_REQTREE_PREREQS_KNOWN;
    }

    return COLOR_REQTREE_UNKNOWN;
  } else {
    return COLOR_REQTREE_BACKGROUND;
  }

}


/****************************************************************************
  Return the type for an edge between two nodes
  if node is a dummy, dest_node can be NULL
****************************************************************************/
static enum reqtree_edge_type get_edge_type(struct tree_node *node, 
                                            struct tree_node *dest_node)
{
  struct player_research *research = get_player_research(client.conn.playing);

  if (dest_node == NULL) {
    /* assume node is a dummy */
    dest_node = node;
  }
   
  /* find the required tech */
  while (node->is_dummy) {
    assert(node->nrequire == 1);
    node = node->require[0];
  }
  
  /* find destination advance by recursing in dest_node->provide[]
   * watch out: recursion */
  if (dest_node->is_dummy) {
    enum reqtree_edge_type sum_type = REQTREE_EDGE;
    int i;

    assert(dest_node->nprovide > 0);
    for (i = 0; i < dest_node->nprovide; ++i) {
      enum reqtree_edge_type type = get_edge_type(node, dest_node->provide[i]);
      switch (type) {
      case REQTREE_ACTIVE_EDGE:
      case REQTREE_GOAL_EDGE:
        return type;
      case REQTREE_KNOWN_EDGE:
      case REQTREE_READY_EDGE:
        sum_type = type;
        break;
      default:
        /* no change */
        break;
      };
    }
    return sum_type;
  }

  if (!research) {
    /* Global observer case */
    return REQTREE_KNOWN_EDGE;
  }
  
  if (research->researching == dest_node->tech) {
    return REQTREE_ACTIVE_EDGE;
  }

  if (is_tech_a_req_for_goal(client.conn.playing, dest_node->tech,
                             research->tech_goal)
      || dest_node->tech == research->tech_goal) {
    return REQTREE_GOAL_EDGE;
  }

  if (TECH_KNOWN == player_invention_state(client.conn.playing, node->tech)) {
    if (TECH_KNOWN == player_invention_state(client.conn.playing, dest_node->tech)) {
      return REQTREE_KNOWN_EDGE;
    } else {
      return REQTREE_READY_EDGE;
    }
  }

  return REQTREE_EDGE;
}

/****************************************************************************
  Return a stroke color for an edge between two nodes
  if node is a dummy, dest_node can be NULL
****************************************************************************/
static enum color_std edge_color(struct tree_node *node, 
                                 struct tree_node *dest_node)
{
  enum reqtree_edge_type type = get_edge_type(node, dest_node);
  switch (type) {
  case REQTREE_ACTIVE_EDGE:
    return COLOR_REQTREE_RESEARCHING;
  case REQTREE_GOAL_EDGE:
    return COLOR_REQTREE_GOAL_UNKNOWN;
  case REQTREE_KNOWN_EDGE:
    /* using "text" black instead of "known" white/ground/green */
    return COLOR_REQTREE_TEXT;
  case REQTREE_READY_EDGE:
    return COLOR_REQTREE_PREREQS_KNOWN;
  default:
    return COLOR_REQTREE_EDGE;
  };
}

/****************************************************************************
  Draw the reqtree diagram!

  This draws the given portion of the reqtree diagram (given by
  (tt_x,tt_y) and (w,h) onto the canvas at position (canvas_x, canvas_y).
****************************************************************************/
void draw_reqtree(struct reqtree *tree, struct canvas *pcanvas,
		  int canvas_x, int canvas_y,
		  int tt_x, int tt_y, int w, int h)
{
  int i, j, k;
  int swidth, sheight;
  struct sprite* sprite;
  struct color *color;

  /* draw the diagram */
  for (i = 0; i < tree->num_layers; i++) {
    for (j = 0; j < tree->layer_size[i]; j++) {
      struct tree_node *node = tree->layers[i][j];
      int startx, starty, endx, endy, width, height;

      startx = node->node_x;
      starty = node->node_y;
      width = node->node_width;
      height = node->node_height;

      if (node->is_dummy) {
        /* Use the same layout as lines for dummy nodes */
        canvas_put_line(pcanvas,
		        get_color(tileset, edge_color(node, NULL)),
		        LINE_GOTO,
		        startx, starty, width, 0);
      } else {
	const char *text = advance_name_for_player(client.conn.playing, node->tech);
	int text_w, text_h;
	int icon_startx;
	
        canvas_put_rectangle(pcanvas,
                             get_color(tileset, COLOR_REQTREE_BACKGROUND),
                             startx, starty, width, height);

	/* Print color rectangle with text inside. */
	canvas_put_rectangle(pcanvas, get_color(tileset, node_color(node)),
			     startx + 1, starty + 1,
			     width - 2, height - 2);
	/* The following code is similar to the one in 
	 * node_rectangle_minimum_size(). If you change something here,
	 * change also node_rectangle_minimum_size().
	 */
			     
	get_text_size(&text_w, &text_h, FONT_REQTREE_TEXT, text);

	canvas_put_text(pcanvas,
			startx + (width - text_w) / 2,
			starty + 4,
			FONT_REQTREE_TEXT,
			get_color(tileset, COLOR_REQTREE_TEXT),
			text);
 	icon_startx = startx + 5;
	
	if (reqtree_show_icons) {
 	  unit_type_iterate(unit) {
            if (advance_number(unit->require_advance) != node->tech) {
	      continue;
	    }
 	    sprite = get_unittype_sprite(tileset, unit);
 	    get_sprite_dimensions(sprite, &swidth, &sheight);
 	    canvas_put_sprite_full(pcanvas,
 	                           icon_startx,
 				   starty + text_h + 4
 				   + (height - text_h - 4 - sheight) / 2,
 				   sprite);
 	    icon_startx += swidth + 2;
 	  } unit_type_iterate_end;
       
          improvement_iterate(pimprove) {
            requirement_vector_iterate(&(pimprove->reqs), preq) {
              if (VUT_ADVANCE == preq->source.kind
	       && advance_number(preq->source.value.advance) == node->tech) {
 	        sprite = get_building_sprite(tileset, pimprove);
                /* Improvement icons are not guaranteed to exist */
                if (sprite) {
                  get_sprite_dimensions(sprite, &swidth, &sheight);
                  canvas_put_sprite_full(pcanvas,
                                         icon_startx,
                                         starty + text_h + 4
                                         + (height - text_h - 4 - sheight) / 2,
                                         sprite);
                  icon_startx += swidth + 2;
                }
 	      }
 	    } requirement_vector_iterate_end;
          } improvement_iterate_end;

          government_iterate(gov) {
            requirement_vector_iterate(&(gov->reqs), preq) {
              if (VUT_ADVANCE == preq->source.kind
               && advance_number(preq->source.value.advance) == node->tech) {
                sprite = get_government_sprite(tileset, gov);
                get_sprite_dimensions(sprite, &swidth, &sheight);
 	        canvas_put_sprite_full(pcanvas,
 	                               icon_startx,
 				       starty + text_h + 4
 				       + (height - text_h - 4 - sheight) / 2,
 	                               sprite);
 	        icon_startx += swidth + 2;
              }
            } requirement_vector_iterate_end;
          } government_iterate_end;
        }
      }

      /* Draw all outgoing edges */
      startx = node->node_x + node->node_width;
      starty = node->node_y + node->node_height / 2;
      for (k = 0; k < node->nprovide; k++) {
	struct tree_node *dest_node = node->provide[k];
	color = get_color(tileset, edge_color(node, dest_node));

	endx = dest_node->node_x;
	endy = dest_node->node_y + dest_node->node_height / 2;
	
        if (reqtree_curved_lines) {
          canvas_put_curved_line(pcanvas, color, LINE_GOTO,
                                 startx, starty, endx - startx,
                                 endy - starty);
        } else {
          canvas_put_line(pcanvas, color, LINE_GOTO,
                          startx, starty, endx - startx,
                          endy - starty);
        }
      }
    }
  }
}

/****************************************************************************
  Return the tech ID at the given position of the reqtree (or A_NONE).
****************************************************************************/
Tech_type_id get_tech_on_reqtree(struct reqtree *tree, int x, int y)
{
  int i;

  for (i = 0; i < tree->num_nodes; i++) {
    struct tree_node *node = tree->nodes[i];

    if (node->is_dummy) {
      continue;
    }
    if (node->node_x <= x && node->node_y <= y
        && node->node_x + node->node_width > x
	&& node->node_y + node->node_height > y) {
      return node->tech;
    }
  }
  return A_NONE;
}

/****************************************************************************
  Return the position of the given tech on the reqtree.  Return TRUE iff
  it was found.
****************************************************************************/
bool find_tech_on_reqtree(struct reqtree *tree, Tech_type_id tech,
			  int *x, int *y, int *w, int *h)
{
  int i;

  for (i = 0; i < tree->num_nodes; i++) {
    struct tree_node *node = tree->nodes[i];

    if (!node->is_dummy && node->tech == tech) {
      if (x) {
	*x = node->node_x;
      }
      if (y) {
	*y = node->node_y;
      }
      if (w) {
	*w = node->node_width;
      }
      if (h) {
	*h = node->node_height;
      }
      return TRUE;
    }
  }
  return FALSE;
}
