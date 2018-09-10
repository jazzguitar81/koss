/* Alpabet Order */
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kref.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#define DRIVER_DESC	"Work Queue Test Kernel Module"

struct test_work {
	atomic_t index;
	int	type;

	struct kref kref;

	struct work_struct	w;
	struct workqueue_struct *wq;
};
struct test_work *tw;

enum {
	TYPE_NONE,
	TYPE_WQ,			/* create_workqueue() */
	TYPE_SINGLE_WQ,			/* create_singlethread_workqueue() */
	TYPE_ALLOC_WQ,			/* alloc_workqueue() */
	TYPE_ALLOC_WQ_MAX,		/* alloc_workqueue() with max_active */
	TYPE_ALLOC_WQ_UNBOUND,		/* alloc_workqueue() unbound */

	TYPE_ALLOC_WQ_UNBOUND_MAX,	/* alloc_workqueue() unbound & max_active */
	TYPE_ALLOC_WQ_NONE,
	TYPE_MAX,
};

static int wq_type = 0;

module_param(wq_type, int, 0644);
MODULE_PARM_DESC(wq_type, "create work queue type for test purpose");

/********** debugfs **********/
static struct dentry *dent;

static int type_show(struct seq_file *s, void *unused)
{
	struct test_work *tw = s->private;

	switch (tw->type) {
	case TYPE_WQ:
		seq_puts(s, "type: create_workqueue()\n");
		break;
	case TYPE_SINGLE_WQ:
		seq_puts(s, "type: create_singlethread_workqueue()\n");
		break;
	case TYPE_ALLOC_WQ:
		seq_puts(s, "type: alloc_workqueue() with single\n");
		break;
	case TYPE_ALLOC_WQ_MAX:
		seq_puts(s, "type: alloc_workqueue() with max_active(256)\n");
		break;
	case TYPE_ALLOC_WQ_UNBOUND:
		seq_puts(s, "type: alloc_workqueue() with unbound\n");
		break;
	case TYPE_ALLOC_WQ_UNBOUND_MAX:
		seq_puts(s, "type: alloc_workqueue() with unbound & max_active\n");
		break;
	case TYPE_ALLOC_WQ_NONE:
		seq_puts(s, "type: alloc_workqueue() with none\n");
		break;
	default:
		seq_puts(s, "No value");
		break;
	}

	return 0;
}

static int type_open(struct inode *inode, struct file *file)
{
	return single_open(file, type_show, inode->i_private);
}

static ssize_t type_write(struct file *file, const char __user *ubuf,
			  size_t count, loff_t *ppos)
{
	pr_info("%s: called\n", __func__);

	/* Not yet implement */
	return count;
}

static const struct file_operations type_fops = {
	.open	= type_open,
	.read	= seq_read,
	.write	= type_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int start_show(struct seq_file *s, void *unused)
{
	struct test_work *tw = s->private;
	int i;
	bool result;

	pr_info("%s: called\n", __func__);

	if (wq_type == 0) {
		pr_err("%s: You need to set wq_type (1~6)\n", __func__);
		return 0;
	}

	/* Let's put the queue_work here for test */
	for (i = 0; i < 10; i++) {
		pr_err("%s: (%d)th: Add test_work to the tw's workqueue\n",
					__func__, i+1);
		result = queue_work(tw->wq, &tw->w);
		if (result == false)
			pr_err("%s: (%d)th: failed to queue_work\n",
					__func__, i+1);
		msleep(20);
	}

	pr_info("%s: done ---->\n", __func__);

	return 0;
}

void func_b(void)
{
	pr_info("%s: called\n", __func__);
}

void func_a(void)
{
	pr_info("%s: called\n", __func__);
	func_b();
}

static int start_open(struct inode *inode, struct file *file)
{
	func_a();
	return single_open(file, start_show, inode->i_private);
}

static const struct file_operations start_fops = {
	.open = start_open,
	.release = single_release,
	.read = seq_read,
	.llseek = seq_lseek,
};

static void work_debugfs_init(struct test_work *tw)
{
	struct dentry *file;

	dent = debugfs_create_dir("test_work", 0);
	if (IS_ERR_OR_NULL(dent)) {
		pr_err("%s: failed to debugfs dir create\n", __func__);
		return;
	}

	file = debugfs_create_file("type", 0666, dent, tw, &type_fops);
	if (IS_ERR_OR_NULL(file)) {
		pr_err("%s: failed to debugfs file(type) create\n", __func__);
		debugfs_remove_recursive(dent);
	}

	file = debugfs_create_file("start", 0644, dent, tw, &start_fops);
	if (IS_ERR_OR_NULL(file)) {
		pr_err("%s: failed to debugfs file(start) create\n", __func__);
		debugfs_remove_recursive(dent);
	}

	return;
}

static void work_debugfs_remove(void)
{
	debugfs_remove_recursive(dent);
}
/********** debugfs **********/

static void work_func(struct work_struct *work)
{
	struct test_work *tw;

	tw = container_of(work, struct test_work, w);
	if (!tw) {
		pr_err("%s: Null pointer!!\n", __func__);
		return;
	}

	atomic_inc(&tw->index);

	pr_info("%s: (%d)th: start---> ", __func__, atomic_read(&tw->index));

	pr_info("%s: (%d)th: step1. 500ms sleep.\n", __func__, atomic_read(&tw->index));
	msleep(500);

	pr_info("%s: (%d)th: step2. 50ms delay\n", __func__, atomic_read(&tw->index));
	mdelay(50);

	pr_info("%s: (%d)th: step3. 500ms sleep\n", __func__, atomic_read(&tw->index));
	msleep(500);

	pr_info("%s: (%d)th: done----> ", __func__, atomic_read(&tw->index));
}

static void work_func_with_busy(struct work_struct *work)
{
	struct test_work *tw;

	tw = container_of(work, struct test_work, w);
	if (!tw) {
		pr_err("%s: Null pointer!!\n", __func__);
		return;
	}

	atomic_inc(&tw->index);

	pr_info("%s: (%d)th: start---> ", __func__, atomic_read(&tw->index));

	pr_info("%s: (%d)th: step2. 50ms delay\n", __func__, atomic_read(&tw->index));
	mdelay(50);

	pr_info("%s: (%d)th: done----> ", __func__, atomic_read(&tw->index));
}

static void __init_wq(void)
{
	switch (wq_type) {
	case TYPE_WQ:
		pr_info("%s: create_singlethread_workqueue()\n", __func__);
		tw->wq = create_singlethread_workqueue("test_wq");
		break;
	case TYPE_SINGLE_WQ:
		pr_info("%s: create_workqueue()\n", __func__);
		tw->wq = create_workqueue("test_wq");
		break;
	case TYPE_ALLOC_WQ:
		pr_info("%s: alloc_workqueue() with sigle\n", __func__);
		tw->wq = alloc_workqueue("test_wq", WQ_HIGHPRI | WQ_CPU_INTENSIVE, 10);
		break;
	case TYPE_ALLOC_WQ_MAX:
		pr_info("%s: alloc_workqueue() with max_active(256)\n", __func__);
		/* max_active is 0, which means set as 256 for max_active. */
		tw->wq = alloc_workqueue("test_wq", WQ_HIGHPRI | WQ_CPU_INTENSIVE, 0);
		break;
	case TYPE_ALLOC_WQ_UNBOUND:
		pr_info("%s: alloc_workqueue() with unbound\n", __func__);
		tw->wq = alloc_workqueue("test_wq", WQ_HIGHPRI | WQ_UNBOUND, 1);
		break;
	case TYPE_ALLOC_WQ_UNBOUND_MAX:
		pr_info("%s: alloc_workqueue() with unbound & max_active(256)\n", __func__);
		tw->wq = alloc_workqueue("test_wq", WQ_HIGHPRI | WQ_UNBOUND, 0);
		break;
	case TYPE_ALLOC_WQ_NONE:
		pr_info("%s: alloc_workqueue() with none\n", __func__);
		tw->wq = alloc_workqueue("test_wq", 0, 0);
		break;
	default:
		pr_err("%s:Not yet value\n", __func__);
		return;
	}

	//INIT_WORK(&tw->w, work_func);
	INIT_WORK(&tw->w, work_func_with_busy);
}

static void cleanup_wq(struct kref *refcount)
{
	struct test_work *tw;

	tw = container_of(refcount, struct test_work, kref);
	if (tw->wq) {
		flush_workqueue(tw->wq);
		destroy_workqueue(tw->wq);
	}
	work_debugfs_remove();
	kfree(tw);
}

static int __init init_wq(void)
{
	tw = kzalloc(sizeof(struct test_work), GFP_KERNEL);
	if (!tw)
		return -ENOMEM;

	work_debugfs_init(tw);
	__init_wq();
	atomic_set(&tw->index, 0);

	kref_init(&tw->kref);

	pr_info("%s: test workqueu module init\n", __func__);

	return 0;
}

static void __exit exit_wq(void)
{
	pr_info("%s: called\n", __func__);
	kref_put(&tw->kref, cleanup_wq);
}

module_init(init_wq);
module_exit(exit_wq);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_DESC);
