
#include <string.h>
#include "m_pd.h"

/* -------------------- receiver ------------------------------ */

static t_class *receiver_class;
static t_class *receiver_proxy_class;

typedef struct _receiver_proxy
{
    t_pd              p_pd;
    struct _receiver  *p_owner;
} t_receiver_proxy;

typedef struct _receiver
{
    t_object         x_obj;
    t_receiver_proxy  x_proxy;
    t_symbol        *x_sym;
} t_receiver;

static void receiver_proxy_init(t_receiver_proxy * p, t_receiver *x)
{
    p->p_pd = receiver_proxy_class;
    p->p_owner = x;
}

static void receiver_proxy_symbol(t_receiver_proxy *p, t_symbol* s)
{
    t_receiver *x = p->p_owner;
    pd_unbind(&x->x_obj.ob_pd, x->x_sym);
    pd_bind(&x->x_obj.ob_pd, x->x_sym = s);
}

static void receiver_bang(t_receiver *x)
{
    outlet_bang(x->x_obj.ob_outlet);
}

static void receiver_float(t_receiver *x, t_float f)
{
    outlet_float(x->x_obj.ob_outlet, f);
}

static void receiver_symbol(t_receiver *x, t_symbol *s)
{
    outlet_symbol(x->x_obj.ob_outlet, s);
}

static void receiver_pointer(t_receiver *x, t_gpointer *gp)
{
    outlet_pointer(x->x_obj.ob_outlet, gp);
}

static void receiver_list(t_receiver *x, t_symbol *s, int argc, t_atom *argv)
{
    outlet_list(x->x_obj.ob_outlet, s, argc, argv);
}

static void receiver_anything(t_receiver *x, t_symbol *s, int argc, t_atom *argv)
{
    outlet_anything(x->x_obj.ob_outlet, s, argc, argv);
}

static void *receiver_new(t_symbol *s)
{
    t_receiver *x = (t_receiver *)pd_new(receiver_class);
    x->x_sym = s;
    pd_bind(&x->x_obj.ob_pd, s);
    if (!*x->x_sym->s_name)
    {
        receiver_proxy_init(&x->x_proxy, x);
        inlet_new(&x->x_obj, &x->x_proxy.p_pd, 0, 0);
    }
    outlet_new(&x->x_obj, 0);
    return (x);
}

static void receiver_free(t_receiver *x)
{
    pd_unbind(&x->x_obj.ob_pd, x->x_sym);
}

void receiver_setup(void)
{
    receiver_class = class_new(gensym("receiver"), (t_newmethod)receiver_new,
            (t_method)receiver_free, sizeof(t_receiver), CLASS_NOINLET, A_DEFSYM, 0);
    class_addbang(receiver_class, receiver_bang);
    class_addfloat(receiver_class, (t_method)receiver_float);
    class_addsymbol(receiver_class, receiver_symbol);
    class_addpointer(receiver_class, receiver_pointer);
    class_addlist(receiver_class, receiver_list);
    class_addanything(receiver_class, receiver_anything);
    receiver_proxy_class = (t_class *)class_new(gensym("receiver proxy"),
                                    0, 0, sizeof(t_receiver_proxy), 0, 0);
    class_addsymbol(receiver_proxy_class, receiver_proxy_symbol);
}
