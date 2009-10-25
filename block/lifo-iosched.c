/*
 * LIFO I/O scheduler (based on no-op)
 * written by Miguel Boton
 */

#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/init.h>

struct lifo_data {
	struct list_head queue;
};

static void lifo_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static int lifo_dispatch(struct request_queue *q, int force)
{
	struct lifo_data *ld = q->elevator->elevator_data;

	if (!list_empty(&ld->queue)) {
		struct request *rq;
		rq = list_entry(ld->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		return 1;
	}
	return 0;
}

static void lifo_add_request(struct request_queue *q, struct request *rq)
{
	struct lifo_data *ld = q->elevator->elevator_data;

	list_add(&rq->queuelist, &ld->queue);
}

static int lifo_queue_empty(struct request_queue *q)
{
	struct lifo_data *ld = q->elevator->elevator_data;

	return list_empty(&ld->queue);
}

static struct request *
lifo_former_request(struct request_queue *q, struct request *rq)
{
	struct lifo_data *ld = q->elevator->elevator_data;

	if (rq->queuelist.prev == &ld->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
lifo_latter_request(struct request_queue *q, struct request *rq)
{
	struct lifo_data *ld = q->elevator->elevator_data;

	if (rq->queuelist.next == &ld->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static void *lifo_init_queue(struct request_queue *q)
{
	struct lifo_data *ld;

	ld = kmalloc_node(sizeof(*ld), GFP_KERNEL, q->node);
	if (!ld)
		return NULL;

	INIT_LIST_HEAD(&ld->queue);
	return ld;
}

static void lifo_exit_queue(struct elevator_queue *e)
{
	struct lifo_data *ld = e->elevator_data;

	BUG_ON(!list_empty(&ld->queue));
	kfree(ld);
}

static struct elevator_type elevator_lifo = {
	.ops = {
		.elevator_merge_req_fn		= lifo_merged_requests,
		.elevator_dispatch_fn		= lifo_dispatch,
		.elevator_add_req_fn		= lifo_add_request,
		.elevator_queue_empty_fn	= lifo_queue_empty,
		.elevator_former_req_fn		= lifo_former_request,
		.elevator_latter_req_fn		= lifo_latter_request,
		.elevator_init_fn		= lifo_init_queue,
		.elevator_exit_fn		= lifo_exit_queue,
	},
	.elevator_name = "lifo",
	.elevator_owner = THIS_MODULE,
};

static int __init lifo_init(void)
{
	elv_register(&elevator_lifo);

	return 0;
}

static void __exit lifo_exit(void)
{
	elv_unregister(&elevator_lifo);
}

module_init(lifo_init);
module_exit(lifo_exit);


MODULE_AUTHOR("Miguel Boton");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LIFO IO scheduler");
