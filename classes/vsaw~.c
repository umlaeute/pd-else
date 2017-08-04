// Porres 2017

#include "m_pd.h"
#include "math.h"


static t_class *vsaw_class;

typedef struct _vsaw
{
    t_object x_obj;
    double  x_phase;
    double  x_last_phase_offset;
    t_float  x_freq;
    t_inlet  *x_inlet_width;
    t_inlet  *x_inlet_phase;
    t_inlet  *x_inlet_sync;
    t_outlet *x_outlet;
    t_float x_sr;
} t_vsaw;

static t_int *vsaw_perform(t_int *w)
{
    t_vsaw *x = (t_vsaw *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // freq
    t_float *in2 = (t_float *)(w[4]); // width
    t_float *in3 = (t_float *)(w[5]); // phase
    t_float *in4 = (t_float *)(w[6]); // sync
    t_float *out = (t_float *)(w[7]);
    double phase = x->x_phase;
    double last_phase_offset = x->x_last_phase_offset;
    double sr = x->x_sr;
    double output;
    while (nblock--)
    {
        double hz = *in1++;
        double width = *in2++;
        width = width > 1. ? 1. : width < -1. ? -1. : width; // clipped
        double phase_offset = *in3++;
        double trig = *in4++;
        double phase_step = hz / sr; // phase_step
        phase_step = phase_step > 0.5 ? 0.5 : phase_step < -0.5 ? -0.5 : phase_step; // clipped to nyq
        double phase_dev = phase_offset - last_phase_offset;
        if (phase_dev >= 1 || phase_dev <= -1)
            phase_dev = fmod(phase_dev, 1); // fmod(phase_dev)

        if (trig > 0 && trig < 1)
            phase = trig;
        else if (trig == 1)
            phase = 0;
        else
            {
            phase = phase + phase_dev;
            if (phase <= 0) phase = phase + 1.; // wrap deviated phase
            if (phase >= 1) phase = phase - 1.; // wrap deviated phase
            }
        
        if (width == -1) output = phase * -2 + 1;
        else if (width == 1) output = phase * 2 - 1;
        else
            {
                width = (width + 1) * 0.5;
                t_float inc = phase * width;                   // phase * 0.5
                t_float dec = (phase - 1) * (width - 1);       //
                t_float gain = pow(width * (width - 1), -1);
                t_float min = (inc < dec ? inc : dec);
                output = (min * gain) * 2 + 1;
            }
        *out++ = output;
        phase = phase + phase_step; // next phase
        last_phase_offset = phase_offset; // last phase offset
    }
    x->x_phase = phase;
    x->x_last_phase_offset = last_phase_offset;
    return (w + 8);
}

static void vsaw_dsp(t_vsaw *x, t_signal **sp)
{
    dsp_add(vsaw_perform, 7, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
}

static void *vsaw_free(t_vsaw *x)
{
    inlet_free(x->x_inlet_width);
    inlet_free(x->x_inlet_phase);
    inlet_free(x->x_inlet_sync);
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *vsaw_new(t_symbol *s, int ac, t_atom *av)
{
    t_vsaw *x = (t_vsaw *)pd_new(vsaw_class);
    t_float f1 = 0, f2 = 0.5, f3 = 0;
    if (ac && av->a_type == A_FLOAT)
    {
        f1 = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT)
            f2 = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT)
            f3 = av->a_w.w_float;
    }
    
    t_float init_freq = f1;
    
    t_float init_width = f2;
    init_width < -1 ? -1 : init_width > 1 ? 1 : init_width; // clipping width input
    
    t_float init_phase = f3;
    init_phase < 0 ? 0 : init_phase >= 1 ? 0 : init_phase; // clipping phase input
    if (init_phase == 0 && init_freq > 0)
        x->x_phase = 1.;
    
    x->x_last_phase_offset = 0;
    x->x_freq = init_freq;
    x->x_sr = sys_getsr(); // sample rate

    x->x_inlet_width = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_width, init_width);
    
    x->x_inlet_phase = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_phase, init_phase);
    
    x->x_inlet_sync = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_sync, 0);
    
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void vsaw_tilde_setup(void)
{
    vsaw_class = class_new(gensym("vsaw~"),
        (t_newmethod)vsaw_new, (t_method)vsaw_free,
        sizeof(t_vsaw), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(vsaw_class, t_vsaw, x_freq);
    class_addmethod(vsaw_class, (t_method)vsaw_dsp, gensym("dsp"), A_CANT, 0);
}