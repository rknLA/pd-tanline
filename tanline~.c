/* Copyright (c) 2017 Kevin Nelson */

#include "m_pd.h"  
#include "math.h"

/* -------------------------- tanline~ ------------------------------ */
/* modeled after the binops: if no argument is given, there are two signal
 * inlets for vector/vector operation; otherwise, vector scalar and the
 * second inlet takes a float value.
 *
 * vector/vector in this case means that the amount can modulate at 
 * signal rate
 */
static t_class *tanline_class, *scalar_tanline_class;

typedef struct _tanline
{
    t_object x_obj;
    t_float x_sig_dummy;
} t_tanline;

typedef struct _scalar_tanline
{
    t_object x_obj;
    t_float x_sig_dummy;

    /* amount of tanh to apply, user param should be between 0.1 and 2.0 */
    t_float x_tan_amount;
} t_scalar_tanline;

static void *tanline_new(t_symbol *s, int argc, t_atom *argv)
{
    if (argc > 1) post("tanline~: extra arguments ignored");
    if (argc) {
        t_scalar_tanline *x = (t_scalar_tanline *)pd_new(scalar_tanline_class);
        floatinlet_new(&x->x_obj, &x->x_tan_amount);
        x->x_tan_amount = atom_getfloatarg(0, argc, argv);
        outlet_new(&x->x_obj, &s_signal);
        x->x_sig_dummy = 0;
        return (x);
    } else {
        t_tanline *x = (t_tanline *)pd_new(tanline_class);
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
        outlet_new(&x->x_obj, &s_signal);
        x->x_sig_dummy = 0;
        return (x);
    }
}

static inline t_sample apply_tanh(t_sample sig, t_float amt) {
    float ebx = pow(M_E, (amt * sig));
    float numer = 2 * ebx - 2;
    float denom = amt * ebx + amt;
    return (numer / denom);
}

static t_int *tanline_perform(t_int *w)
{
    t_sample *sig_in = (t_sample *)(w[1]);
    t_sample *amt_in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    while (n--) *out++ = apply_tanh(*sig_in++, *amt_in++);
    return (w+5);
}

static t_int *tanline_perf8(t_int *w)
{
    t_sample *sig_in = (t_sample *)(w[1]);
    t_sample *amt_in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    for (; n; n -= 8, sig_in += 8, amt_in += 8, out += 8) {
        t_sample s0 = sig_in[0], s1 = sig_in[1], s2 = sig_in[2], s3 = sig_in[3];
        t_sample s4 = sig_in[4], s5 = sig_in[5], s6 = sig_in[6], s7 = sig_in[7];

        t_sample a0 = amt_in[0], a1 = amt_in[1], a2 = amt_in[2], a3 = amt_in[3];
        t_sample a4 = amt_in[4], a5 = amt_in[5], a6 = amt_in[6], a7 = amt_in[7];

        out[0] = apply_tanh(s0, a0); out[1] = apply_tanh(s1, a1); out[2] = apply_tanh(s2, a2); out[3] = apply_tanh(s3, a3);
        out[4] = apply_tanh(s4, a4); out[5] = apply_tanh(s5, a5); out[6] = apply_tanh(s6, a6); out[7] = apply_tanh(s7, a7);
    }
    return (w+5);
}

static t_int *scalar_tanline_perform(t_int *w)
{
    t_sample *sig_in = (t_sample *)(w[1]);
    t_float amt = *(t_float *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    while (n--) *out++ = apply_tanh(*sig_in++, amt);
    return (w+5);
}

static t_int *scalar_tanline_perf8(t_int *w)
{
    t_sample *sig_in = (t_sample *)(w[1]);
    t_float amt = *(t_float *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    for (; n; n -= 8, sig_in += 8, out += 8) {
        t_sample s0 = sig_in[0], s1 = sig_in[1], s2 = sig_in[2], s3 = sig_in[3];
        t_sample s4 = sig_in[4], s5 = sig_in[5], s6 = sig_in[6], s7 = sig_in[7];

        out[0] = apply_tanh(s0, amt); out[1] = apply_tanh(s1, amt); out[2] = apply_tanh(s2, amt); out[3] = apply_tanh(s3, amt);
        out[4] = apply_tanh(s4, amt); out[5] = apply_tanh(s5, amt); out[6] = apply_tanh(s6, amt); out[7] = apply_tanh(s7, amt);
    }
    return (w+5);
}

static void tanline_dsp(t_tanline *x, t_signal **sp)
{
    if (sp[0]->s_n & 7)
        dsp_add(tanline_perform, 4,
                sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
    else
        dsp_add(tanline_perf8, 4,
                sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

static void scalar_tanline_dsp(t_scalar_tanline *x, t_signal **sp)
{
    if (sp[0]->s_n & 7)
        dsp_add(scalar_tanline_perform, 4,
                sp[0]->s_vec, &x->x_tan_amount, sp[2]->s_vec, sp[0]->s_n);
    else
        dsp_add(scalar_tanline_perf8, 4,
                sp[0]->s_vec, &x->x_tan_amount, sp[2]->s_vec, sp[0]->s_n);
}

void tanline_tilde_setup(void)
{
    tanline_class = class_new(gensym("tanline~"), (t_newmethod)tanline_new, 0,
            sizeof(t_tanline), 0, A_GIMME, 0);
    class_addmethod(tanline_class, (t_method)tanline_dsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(tanline_class, t_tanline, x_sig_dummy);

    scalar_tanline_class = class_new(gensym("tanline~"), 0, 0,
            sizeof(t_scalar_tanline), 0, 0);
    CLASS_MAINSIGNALIN(scalar_tanline_class, t_scalar_tanline, x_sig_dummy);
    class_addmethod(scalar_tanline_class, (t_method)scalar_tanline_dsp,
            gensym("dsp"), A_CANT, 0);
}
