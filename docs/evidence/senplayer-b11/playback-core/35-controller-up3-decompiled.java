package defpackage;

import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.view.Display;
import android.view.Surface;
import com.mplaykit.Player;
import java.io.File;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Proxy;
import java.net.ProxySelector;
import java.net.SocketAddress;
import java.net.URI;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;
import java.util.function.BiConsumer;
import java.util.function.Function;
import java.util.function.Supplier;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/* JADX INFO: loaded from: classes.dex */
public final class up3 {
    public final JSONObject A;
    public final xb3 A0;
    public String A1;
    public final kf1 B;
    public final xb3 B0;
    public long B1;
    public final kh3 C;
    public final xb3 C0;
    public kf1 C1;
    public final ek3 D;
    public final AtomicLong D0;
    public final ag1 E;
    public final ks5 E0;
    public final mf1 F;
    public final xb3 F0;
    public final mf1 G;
    public final xb3 G0;
    public final mf1 H;
    public final xb3 H0;
    public final pj3 I;
    public volatile long I0;
    public String J;
    public boolean J0;
    public String K;
    public long K0;
    public final xb3 L;
    public long L0;
    public final wu4 M;
    public boolean M0;
    public final uo3 N;
    public long N0;
    public final uo3 O;
    public int O0;
    public JSONObject P;
    public int P0;
    public JSONObject Q;
    public int Q0;
    public JSONObject R;
    public final Handler R0;
    public final JSONObject S;
    public final nv1 S0;
    public final d T;
    public final ExecutorService T0;
    public final LinkedHashMap U;
    public final AtomicInteger U0;
    public final LinkedHashMap V;
    public boolean V0;
    public final LinkedHashMap W;
    public boolean W0;
    public final xb3 X;
    public final CompletableFuture X0;
    public Map Y;
    public final CompletableFuture Y0;
    public String Z;
    public vk3 Z0;
    public String a;
    public Map a0;
    public boolean a1;
    public final String b;
    public kp3 b0;
    public boolean b1;
    public final String c;
    public volatile boolean c0;
    public final AtomicBoolean c1;
    public final String d;
    public long d0;
    public Exception d1;
    public final double e;
    public final xb3 e0;
    public Surface e1;
    public final boolean f;
    public final xb3 f0;
    public final xb3 f1;
    public final double g;
    public final xb3 g0;
    public final xb3 g1;
    public final kf1 h;
    public pi3 h0;
    public final xb3 h1;
    public final kf1 i;
    public final xb3 i0;
    public final xb3 i1;
    public final kf1 j;
    public final xb3 j0;
    public final xb3 j1;
    public final boolean k;
    public final xb3 k0;
    public Display k1;
    public final mf1 l;
    public String l0;
    public Surface l1;
    public Map m;
    public final ki m0;
    public int m1;
    public final String n;
    public final xb3 n0;
    public int n1;
    public final hi3 o;
    public final xb3 o0;
    public boolean o1;
    public boolean p;
    public final xb3 p0;
    public boolean p1;
    public final double q;
    public final xb3 q0;
    public boolean q1;
    public final String r;
    public final xx4 r0;
    public Double r1;
    public final tf3 s;
    public final xb3 s0;
    public Player s1;
    public final mf1 t;
    public final xb3 t0;
    public long t1;
    public final xf3 u;
    public final xb3 u0;
    public boolean u1;
    public final gg3 v;
    public final vb3 v0;
    public int v1;
    public final JSONObject w;
    public long w0;
    public qp3 w1;
    public final JSONObject x;
    public int x0;
    public final uo3 x1;
    public final JSONObject y;
    public long y0;
    public CompletableFuture y1;
    public final fn3 z;
    public int z0;
    public final eg3 z1;

    /* JADX WARN: Multi-variable type inference failed */
    public up3(pp3 pp3Var, rp3 rp3Var) {
        JSONObject jSONObject;
        JSONObject jSONObject2;
        JSONObject jSONObject3;
        m54.a("s1:CnmTmTBxZ73q8JDqKgOrvIAH3rdHhUskY9ErdgHko9UYCmRbNlUqGA==");
        m54.a("s1:vzZt4svo5UDQ3hkCH+0VGlAmy369skEasmPUUoCEtjyqX+XcmiqyACyLq5I=");
        this.a = pp3Var.a;
        this.b = pp3Var.b;
        this.c = pp3Var.p;
        this.d = pp3Var.c;
        this.e = pp3Var.d;
        this.f = pp3Var.e;
        this.g = pp3Var.f;
        this.h = rp3Var.a;
        this.i = rp3Var.b;
        this.j = rp3Var.c;
        this.k = pp3Var.g;
        this.l = rp3Var.d;
        this.m = pp3Var.i;
        this.n = pp3Var.j;
        hi3 hi3Var = pp3Var.k;
        this.o = hi3Var;
        this.p = hi3Var.f;
        this.q = pp3Var.l;
        this.r = pp3Var.m;
        String str = pp3Var.n;
        this.s = rp3Var.e;
        this.t = rp3Var.f;
        xf3 xf3Var = rp3Var.g;
        this.u = xf3Var;
        this.v = rp3Var.h;
        this.w = rp3Var.k;
        if (xf3Var != null) {
            jSONObject = n86.k(xf3Var.b);
        } else {
            JSONObject jSONObject4 = rp3Var.i;
            jSONObject = jSONObject4 != null ? new JSONObject(jSONObject4.toString()) : null;
        }
        this.x = jSONObject;
        if (xf3Var == null || (jSONObject2 = n86.k(xf3Var.c).getJSONObject(m54.a("s1:TlkSTmPiLWGQ1/AB7mEVOZn5oNPfV1/w8H4hCqSvQx4GGRFY0bqMod3N"))) == null) {
            JSONObject jSONObject5 = rp3Var.j;
            jSONObject2 = jSONObject5 != null ? new JSONObject(jSONObject5.toString()) : null;
        }
        this.y = jSONObject2;
        this.z = rp3Var.l;
        this.A = pp3Var.o;
        this.B = rp3Var.m;
        this.C = rp3Var.n;
        this.D = rp3Var.o;
        this.E = rp3Var.p;
        this.F = rp3Var.r;
        this.G = rp3Var.s;
        this.H = rp3Var.q;
        n nVar = rp3Var.t;
        int i = 2;
        int i2 = 1;
        this.I = nVar != null ? new pj3(nVar, new ko3(i2, rp3Var, this), new yj3(this, i), new yj3(this, 3), new yj3(this, 4), new ln3(this, i)) : null;
        this.L = tl3.n(pp3Var.h);
        this.M = new wu4(new u8(i2, this));
        Object[] objArr = 0;
        this.N = new uo3(this, 0);
        this.O = new uo3(this, i2);
        try {
            jSONObject3 = new JSONObject(str == null ? "{}" : str);
        } catch (Exception unused) {
            qi qiVar = qi.a;
            qi.e(m54.a("s1:gwLuLUmwmaMxvWwQJvCFsaRW/5R70HpWdo0c/jdHWbkuDxfUqXl4wHhtdGgy4OUdSgvHd6nxxZk="), m54.a("s1:vqIi0HxsSL9g5dLHc5fXM7VMqdE9yjjYrneaqaK958RfZMpiP3c586WpL2z6RA=="));
            jSONObject3 = new JSONObject();
        }
        this.S = jSONObject3;
        d dVar = new d(1);
        dVar.b = new JSONObject();
        this.T = dVar;
        this.U = new LinkedHashMap();
        this.V = new LinkedHashMap();
        this.W = new LinkedHashMap();
        tz0 tz0Var = tz0.g;
        this.X = tl3.n(tz0Var);
        this.Y = tz0Var;
        this.Z = "";
        this.a0 = tz0Var;
        this.e0 = tl3.n(new zp3(0L, (String) null, aq3.h(this.e), 0.0d, 0.0d, this.f, false, 0, 0, 0L, (String) null, (Boolean) null, (String) null, (String) null, false, 0L, false, false, false, false, false, false, 0L, 0L, 0L, (String) null, false, 0, (String) null, 1073741755));
        sz0 sz0Var = sz0.g;
        this.f0 = tl3.n(sz0Var);
        this.g0 = tl3.n(sz0Var);
        Boolean bool = Boolean.FALSE;
        this.i0 = tl3.n(bool);
        List list = Collections.EMPTY_LIST;
        this.j0 = tl3.n(new zf3(false, false, list, list));
        this.k0 = tl3.n(null);
        this.l0 = this.r;
        this.m0 = new ki(25);
        this.n0 = tl3.n(null);
        this.o0 = tl3.n(sz0Var);
        this.p0 = tl3.n("");
        this.q0 = tl3.n(Double.valueOf(0.0d));
        this.r0 = new xx4(new yj3(this, 5));
        double d = this.g;
        Double dValueOf = (Math.abs(d) > Double.MAX_VALUE || 0.25d > d || d > 8.0d) ? null : Double.valueOf(d);
        this.s0 = tl3.n(Double.valueOf(dValueOf != null ? dValueOf.doubleValue() : 1.0d));
        this.t0 = tl3.n(null);
        this.u0 = tl3.n(bool);
        this.v0 = new vb3(0L);
        this.x0 = -1;
        this.z0 = -1;
        double d2 = this.q;
        Double dValueOf2 = (Math.abs(d2) > Double.MAX_VALUE || -100.0d > d2 || d2 > 100.0d) ? null : Double.valueOf(d2);
        this.A0 = tl3.n(Double.valueOf(dValueOf2 != null ? dValueOf2.doubleValue() : 0.0d));
        this.B0 = tl3.n(Integer.valueOf(this.o.A));
        this.C0 = tl3.n(null);
        this.D0 = new AtomicLong();
        ks5 ks5Var = new ks5(17, (boolean) (objArr == true ? 1 : 0));
        ks5Var.o = new JSONObject();
        this.E0 = ks5Var;
        this.F0 = tl3.n(new JSONObject());
        this.G0 = tl3.n(null);
        this.H0 = tl3.n(null);
        this.R0 = new Handler(Looper.getMainLooper());
        nv1 nv1Var = new nv1(25);
        this.S0 = nv1Var;
        this.T0 = (ExecutorService) nv1Var.p;
        this.U0 = (AtomicInteger) nv1Var.o;
        this.X0 = new CompletableFuture();
        this.Y0 = new CompletableFuture();
        this.c1 = new AtomicBoolean();
        this.f1 = tl3.n(Integer.valueOf(hi3Var.C));
        this.g1 = tl3.n(Integer.valueOf(hi3Var.B));
        this.h1 = tl3.n(Boolean.valueOf(hi3Var.E));
        this.i1 = tl3.n(m54.a("s1:YijQ907msR/0eaJD6HxhroI97jqrRs2MVWqVMy0gyHj5kA=="));
        this.j1 = tl3.n(bool);
        this.q1 = this.f;
        this.x1 = new uo3(this, i);
        tf3 tf3Var = this.s;
        this.z1 = tf3Var != null ? new eg3(tf3Var, new to3(this), new to3(this)) : null;
        this.A1 = m54.a("s1:3AU+NaKowWZ2k8AcH2Yhmah/t6PkgTbcd+EMtT+Y/APgKQ==");
    }

    public static CompletableFuture c0(up3 up3Var, double d, boolean z, int i) {
        boolean z2;
        CompletableFuture completableFutureE;
        pi3 pi3Var;
        boolean z3 = (i & 2) != 0 ? false : z;
        boolean z4 = true;
        boolean z5 = (i & 4) != 0 ? up3Var.o.e : true;
        boolean z6 = (i & 8) != 0;
        if (up3Var.V0 || !up3Var.K().k() || !up3Var.K().h || up3Var.K().d <= 0.0d || Math.abs(d) > Double.MAX_VALUE) {
            CompletableFuture completableFutureCompletedFuture = CompletableFuture.completedFuture(Boolean.FALSE);
            m54.a("s1:63c8EbeZFLKCiAhHNV/FxKembYZqWbug2i8C51y1E1ZTMLkUqLmF9kBw83uNzby58ESQzVS+pf4YvkFoAR6dwnXWjCI=");
            completableFutureCompletedFuture.getClass();
            return completableFutureCompletedFuture;
        }
        CompletableFuture completableFuture = new CompletableFuture();
        double dF = jg5.f(d, 0.0d, up3Var.K().d);
        if (z6 && (pi3Var = up3Var.h0) != null && !pi3Var.t) {
            try {
                ja4 ja4Var = pi3Var.h;
                double d2 = dF * 1000.0d;
                ja4Var.e(ia4.s, ja4Var.g, new JSONObject().put(m54.a("s1:JmqZBspAZT9rEfdbHnPE7X06+Psrhc1mYg7HdMjuA62Em74XjkoApT+uj4Z1wCI8"), d2));
                pi3Var.i(oa4.a(pi3Var.e(), 0.0d, 0.0d, null, null, null, null, false, false, false, false, 0L, 0.0d, null, null, null, null, false, "", null, 393215));
                qi qiVar = qi.a;
                qi.a(m54.a("s1:T8r2msQVPHLcRnDyWMhdUdWVAHeVgBXP5XYeq8rBUnn1h9rgyf+/3AmN6D4="), m54.a("s1:op5TR2wzcEqsSzyeZamySq0jmKxbKi9x6SO3jQwd2iRSTA5XW/s+m8LXuCjodqa64Q7IHwKkRhFw+4+hKRC9jyApRLRVpGhdZiGm3eDiXJY=") + d2);
            } catch (Exception e) {
                pi3Var.h(m54.a("s1:VprC4T/pfck3KeLEau+UFeT5fza2F/2TzcsoXoePhMpcUobRDh0PyMIasuSh6laIgwNi91r/PDAQnXQ4"), e);
            }
        }
        boolean z7 = z3 && ((Boolean) up3Var.h.a()).booleanValue();
        long jIncrementAndGet = up3Var.D0.incrementAndGet();
        pj3 pj3Var = up3Var.I;
        if (pj3Var != null) {
            pj3Var.k = jIncrementAndGet;
            for (hj3 hj3Var : pj3Var.g) {
                pj3.b(hj3Var);
            }
        }
        ks5 ks5Var = up3Var.E0;
        String strA = m54.a("s1:xgNz5Ah4q01m5Yz23uwObfstBTMdwjqYCj0J9U3BehRklULp");
        JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:cMiK9ljXZ+C3nOB1en3lXyu92gZIj5lxrYUvje/lPERg5b1/f/xwKA=="), dF);
        String strA2 = m54.a("s1:mYyohNyXCXg0pddtRCqsFjbXGD0HjM4OBl6OMGpg0TluGReFCPszRBew3Tc=");
        eg3 eg3Var = up3Var.z1;
        if (eg3Var != null && eg3Var.h(dF * 1000.0d) == null) {
            z4 = false;
        }
        JSONObject jSONObjectV = ks5Var.v(strA, jSONObjectPut.put(strA2, z4));
        long jOptLong = jSONObjectV.optLong(m54.a("s1:lzcC+2AgzunQMQJ86YQVxKsGjE/rePUS+uDKw4QNb/jigd/T7Z73X3tJZg4="));
        up3Var.F0.setValue(y82.o("s1:vDloJ0USn7M0YgPKK45msa8H4YPjQbf5PSUHBIJxYrmj8bPE", "s1:08h3zfqQF/l2d6BTFYhTqHR7dUtuve5E9ob4IClNZdLQB33MuFPrjPDybTdkwyBshNKWYHXMfNufCYzomRJfyA==", jSONObjectV));
        up3Var.f0(new u94(jIncrementAndGet, up3Var.K().a, dF, SystemClock.elapsedRealtime(), false, z5));
        if (z7) {
            up3Var.q1 = false;
        }
        qi qiVar2 = qi.a;
        String strA3 = m54.a("s1:AodeLleWIm9585WmCvU47aiQ87T65WydYYN+lIMLNJwmqNZ94FofFJk7UuI=");
        long j = up3Var.K().a;
        StringBuilder sb = new StringBuilder();
        y82.s("s1:MJCBVc203oRilgRF0NWoAy+/Q/Zynd2Kzkjd90K4EobW8/nWLsJhKWAHf2kdGcgz", sb, j, "s1:dTQZlkWx2islXxJYT5dUUyCjdzJrSkh4YXoX498fyvEGpm9J7aU9/HE6qbU=");
        sb.append(dF);
        sb.append(m54.a("s1:3W81r45/3YT4y0Lo2Jht21Q/Io9ZGzM0YcHpvbYKvUawxae5Z/MKzOSNn28="));
        sb.append(z7);
        sb.append(m54.a("s1:Y6bmIiaAU9j8mwOWAZwPG4vjxAYV5qUaPS1XyuRD6QN1My4X1+0hYO7fUjY+v28Z"));
        sb.append(z5);
        qi.a(strA3, sb.toString());
        final eg3 eg3Var2 = up3Var.z1;
        if (eg3Var2 != null) {
            final double d3 = dF * 1000.0d;
            final boolean z8 = up3Var.q1;
            final boolean z9 = z5;
            z2 = z7;
            completableFutureE = eg3Var2.e(new Supplier() { // from class: bg3
                @Override // java.util.function.Supplier
                public final Object get() {
                    JSONObject jSONObjectK;
                    JSONObject jSONObjectK2;
                    String strOptString;
                    boolean z10;
                    eg3 eg3Var3 = eg3Var2;
                    double d4 = d3;
                    boolean z11 = z9;
                    boolean z12 = z8;
                    synchronized (eg3Var3) {
                        try {
                            yi2 yi2Var = (yi2) eg3Var3.h;
                            synchronized (yi2Var) {
                                jSONObjectK = n86.k((JSONObject) yi2Var.o);
                            }
                            jSONObjectK2 = new yi2(jSONObjectK).K(d4);
                            String strOptString2 = jSONObjectK2.optString(m54.a("s1:VZVGcaIe1ZxoSjO+F8Fh3Jk09o0FjVN52Gs5Htiel4RWzGi9xxB5vg=="));
                            yi2 yi2Var2 = (yi2) eg3Var3.h;
                            synchronized (yi2Var2) {
                                strOptString = ((JSONObject) yi2Var2.p).optString(m54.a("s1:VZVGcaIe1ZxoSjO+F8Fh3Jk09o0FjVN52Gs5Htiel4RWzGi9xxB5vg=="));
                            }
                            z10 = strOptString2.equals(strOptString) && eg3Var3.a > 0;
                        } catch (Throwable th) {
                            throw th;
                        }
                    }
                    if (!z10) {
                        return eg3Var3.b(d4, z12).thenApply((Function) new ev2(1));
                    }
                    to3 to3Var = (to3) eg3Var3.g;
                    double dOptDouble = jSONObjectK2.optDouble(m54.a("s1:JmqZBspAZT9rEfdbHnPE7X06+Psrhc1mYg7HdMjuA62Em74XjkoApT+uj4Z1wCI8"));
                    up3 up3Var2 = to3Var.a;
                    CompletableFuture completableFuture2 = new CompletableFuture();
                    up3Var2.T0.execute(new cp3(up3Var2, up3Var2.U0.get(), completableFuture2, dOptDouble / 1000.0d, z11));
                    return completableFuture2;
                }
            });
        } else {
            CompletableFuture completableFuture2 = new CompletableFuture();
            z2 = z7;
            up3Var.T0.execute(new cp3(up3Var, up3Var.U0.get(), completableFuture2, dF, z5));
            completableFutureE = completableFuture2;
        }
        completableFutureE.whenComplete((BiConsumer) new rw(28, new so3(up3Var, jIncrementAndGet, completableFuture, jOptLong, z2)));
        CompletableFuture completableFutureThenApply = completableFuture.thenApply((Function) new ev2(new z33(16), 27));
        m54.a("s1:CJhNE87w9yxM8Dpjr2E1GnJHXFvgk9eNTf98b96pjTBTvSntg68adRAU+/c7Nr/C2T05nBjAMME=");
        completableFutureThenApply.getClass();
        return completableFutureThenApply;
    }

    /* JADX WARN: Code duplicated, block: B:146:0x03cc  */
    /* JADX WARN: Code duplicated, block: B:153:0x03dc  */
    public static final void m0(up3 up3Var, int i, CompletableFuture completableFuture, Surface surface, Surface surface2, String str, String str2, Display display, int i2, int i3, double d, double d2, double d3, boolean z, int i4, int i5, String str3, Map map, double d4, boolean z2, LinkedHashMap linkedHashMap, String str4, JSONObject jSONObject, JSONObject jSONObject2, JSONObject jSONObject3, JSONObject jSONObject4, zv4 zv4Var) {
        Player player;
        up3 up3Var2;
        int i6;
        Integer num;
        Long lValueOf;
        ArrayList arrayList;
        Object obj;
        boolean z3;
        Integer num2;
        kh3 kh3Var;
        up3 up3Var3 = up3Var;
        int i7 = i;
        try {
            up3Var3.X();
            Exception exc = up3Var3.d1;
            if (exc != null) {
                throw new IllegalStateException(m54.a("s1:NWvxMG+jMqiM7JIIuNdhCP2LwhiSsuvxo+AzkVNMnJdhW5TeHH4mw95QkTTDkjVl+MgqidN5XKWk87sRwcsU04t+"), exc);
            }
            if (up3Var3.U0.get() != i7) {
                if (completableFuture != null) {
                    completableFuture.completeExceptionally(new IllegalStateException(m54.a("s1:HezXQId4yUxDHM6mBjopBeBWF777pNJpKqg7r2JT12exlab6AdNYTk4TaGAe4jQ//hmm+rXBVlLh8Q8psTWL8F/U")));
                    return;
                }
                return;
            }
            if (!surface.isValid() || !surface2.isValid()) {
                throw new IllegalStateException(m54.a("s1:Rspnvi2eugR/cR24Gu65URY2OqTLEqLgpNHPj9vSUx7T/nkS4F7Bdy33wVqFFaoKLwmVLxdHxETWpXMWxaFR4zF0oDI=").toString());
            }
            File file = new File(up3Var3.d, m54.a("s1:TB3z1gLNinOCk6oscZJVH2NbyuBmqZvMZpTiJUOaZrnBRlBNrp3ufHZGrutoEuF2"));
            file.mkdirs();
            ib3 ib3VarR = xp3.r(m54.a("s1:hih8kGvLCOA540IVAsu/kNEMcA3IPhQ1dZrEtAQ63clyMrGAtlGFpFFt2BIRqA=="), file.getAbsolutePath());
            ib3 ib3VarR2 = xp3.r(m54.a("s1:1fJGDtQEWnJF/ld9jnV3LeBc0KQLMaiMWE2kJ3BEtCTidrjHAdufe6V0DXa8KQ5Wk5/Z/Htu05g="), str);
            String strA = m54.a("s1:WC4A2t/pPV/CuN53dW/TB5Ca4RrwmTSAfNKOuGxE5ZKoo0k9url4y22ynf0EcecS9iQlI5hqpPU=");
            kf1 kf1Var = up3Var3.j;
            LinkedHashMap linkedHashMapU = pj2.U(ib3VarR, ib3VarR2, xp3.r(strA, String.valueOf(kf1Var != null ? ((Number) kf1Var.a()).longValue() : ((long) up3Var3.o.g) * 1048576)), xp3.r(m54.a("s1:92UhDjhvJSe+FR+er22B/kIWjVyPR9KUQ+wPm5deIheO+AkZeD5lm44AtZwJlYDI57VOEbxSBT6J0g=="), String.valueOf(up3Var3.o.b())), xp3.r(m54.a("s1:lsvKvUAD/EVpPvfH/97PH4DE3wYuTGwXZMWK1WF8azgIQyPhcEA="), up3Var3.o.k), xp3.r(m54.a("s1:amovbMAATZ7FSRH96Lvazn9cYRkMvPctbSZMoWZ3VarCkVAEonY="), up3Var3.o.l), xp3.r(m54.a("s1:CnmTmTBxZ73q8JDqKgOrvIAH3rdHhUskY9ErdgHko9UYCmRbNlUqGA=="), m54.a("s1:HHLRJP+5KL+ca+61SN1SmHYc2wMUmY5fy7izaWReymleXw==")), xp3.r(m54.a("s1:auGdwADQR0H/5iXeptkTqaJfMQvKSrd5ykUwWGf/kKZ+vEfWqC3MA5K3VNGEh14J"), db4.u.D(up3Var3.d)), xp3.r(m54.a("s1:5MiGhnfgnTizn8y1oosX/gpNPNRFpjeVN4yUIxDHKw5rOZky1VL91ZXxMINRZZc/0D/q5cZS"), m54.a("s1:p9Z11SjOk1MR0IF8uP4ciKKKA8Ob/QiDKE1JRGmSIfhskKNfntLK/HDIEFr0sWJVC7dSPQK6")), xp3.r(m54.a("s1:5tucZ6jlVoW0TUXxR5w2fz/2MKH7dCYPhRML4dpBYRPe6rJUQaBqA6GYhbEGRbagYYjmcg2PaPkinkzHrV4lOxFB2fxRFDQfiso="), m54.a("s1:p9Z11SjOk1MR0IF8uP4ciKKKA8Ob/QiDKE1JRGmSIfhskKNfntLK/HDIEFr0sWJVC7dSPQK6")), xp3.r(m54.a("s1:JPqUUv7L2KPgRhV0M9USeYlTIrnMKbykL6+tfteJIt4ieP0rMzdCCnmR8tqDBjiI+R/iiOXTA6TVgd9H0xo="), m54.a("s1:yP17Mo7RcaqPO76jd2BGeeeiidHeQCx7WSE14n6xt1fL9FtP")), xp3.r(m54.a("s1:NJ2JNjEK+JldiQR3kaxR+w10twMBbVMMF19/FsryUuzPS4GM+K6Rp/5c7HD8eYNYZ4RGtuFZIOCnSbplZdGuywYbHtC1y+xhb33u/skxysFgKg=="), m54.a("s1:yP17Mo7RcaqPO76jd2BGeeeiidHeQCx7WSE14n6xt1fL9FtP")));
            String strT0 = up3Var3.t0(str2);
            if (strT0 != null) {
                linkedHashMapU.put(m54.a("s1:a+uOvldufNasCMQaiJ3ku2utzSQNJhdSrnD2+8Ygf6P60son0bBBifYpgAAE49Zc"), strT0);
            }
            if (!up3Var3.p) {
                linkedHashMapU.put(m54.a("s1:IwTN2MKLbr4tvNytDzbRUIiBrPzUAteYiatpTaGDqbD65SMZEK8="), "no");
            }
            if (up3Var3.k) {
                linkedHashMapU.putAll(pj2.T(xp3.r(m54.a("s1:HqCyikhilK1z9hjH0WxPVZ2BrsLh1BdvR7LVK9f4mYdCc+dZJO7/MM6ISbnhl32VJYs87MlVvaM="), m54.a("s1:l9Uk32gK2RwOC8995NNwMTeap2BqaBddTtW/0OShU9fTdDK3")), xp3.r(m54.a("s1:IwTN2MKLbr4tvNytDzbRUIiBrPzUAteYiatpTaGDqbD65SMZEK8="), m54.a("s1:S9AWje7POdL7d64q2xgKaVLqTqR2kh6k6Rbkibw8ovd5xHhOVbY/fuB1l8BNzRKV")), xp3.r(m54.a("s1:YGzWX7bMiHqOvPgbueRoF/oHokuV52jCz3fR2cko6pYl8C722FKNaiAzf0hd0LooVvv8AX9FihaK+u8BHMgUjWothe47bLQgnVY="), "no")));
            }
            kh3 kh3Var2 = up3Var3.C;
            Player playerD = kh3Var2 != null ? kh3Var2.d(surface, surface2) : null;
            if (playerD == null && (kh3Var = up3Var3.C) != null) {
            }
            if (playerD == null) {
                ek3 ek3Var = up3Var3.D;
                Player player2 = ek3Var != null ? new Player(ek3Var, surface, surface2, display, linkedHashMapU) : null;
                if (player2 == null) {
                    player2 = new Player(surface, surface2, display, linkedHashMapU);
                }
                player = player2;
            } else {
                player = playerD;
            }
            up3Var3.s1 = player;
            if (up3Var3.U0.get() != i7) {
                up3Var3.X();
                return;
            }
            player.B(i2, i3);
            if (playerD == null) {
                try {
                    n0(player, z, i4, i5, up3Var3, i7);
                    up3Var3 = up3Var3;
                } catch (Exception e) {
                    e = e;
                    up3Var3 = up3Var3;
                    i7 = i;
                    up3Var3.X();
                    up3Var3.q(i7, e);
                    return;
                } catch (LinkageError e2) {
                    e = e2;
                    up3Var3 = up3Var3;
                    i7 = i;
                    up3Var3.q(i7, e);
                    return;
                }
            }
            try {
                wh3 wh3Var = new wh3(str2);
                wh3Var.b = str3;
                if (str3.length() > 0) {
                    wh3Var.c = xh3.o;
                }
                wh3Var.f.putAll(map);
                wh3Var.d = d4;
                wh3Var.e = z2;
                long j = 0;
                if (playerD != null) {
                    long jElapsedRealtime = SystemClock.elapsedRealtime();
                    for (String str5 : fm0.K0(m54.a("s1:WC4A2t/pPV/CuN53dW/TB5Ca4RrwmTSAfNKOuGxE5ZKoo0k9url4y22ynf0EcecS9iQlI5hqpPU="), m54.a("s1:lsvKvUAD/EVpPvfH/97PH4DE3wYuTGwXZMWK1WF8azgIQyPhcEA="), m54.a("s1:amovbMAATZ7FSRH96Lvazn9cYRkMvPctbSZMoWZ3VarCkVAEonY="))) {
                        Object obj2 = linkedHashMapU.get(str5);
                        if (obj2 == null) {
                            throw new IllegalStateException(m54.a("s1:vt4o3Zjf9BnrLNsOBVeZwcLDIKoTkuAgmTv4biDhLxsfXmnwF7o1tfN7Dh2jydGgavX08lB7dwdsyBXJtC591zT8BcYB+Da7jGU8Qw==").toString());
                        }
                        player.v(str5, (String) obj2).get();
                    }
                    kh3 kh3Var3 = up3Var3.C;
                    if (kh3Var3 == null) {
                        throw new IllegalStateException(m54.a("s1:vt4o3Zjf9BnrLNsOBVeZwcLDIKoTkuAgmTv4biDhLxsfXmnwF7o1tfN7Dh2jydGgavX08lB7dwdsyBXJtC591zT8BcYB+Da7jGU8Qw==").toString());
                    }
                    up3Var3.t1 = kh3Var3.c(player, wh3Var);
                    tp3 tp3VarP0 = p0(linkedHashMap, player, up3Var3, str4, jSONObject, jSONObject2, jSONObject3, jSONObject4, i, true);
                    boolean z4 = ((nh3) tp3VarP0.e.o) == nh3.o && ((nh3) tp3VarP0.f.o) == nh3.p;
                    if (d != 1.0d && ((num2 = (Integer) player.v(m54.a("s1:EszcE/g81WSB/nKWCB08NUqe+6i3Aay8Q4xE9vWi+dtNYPOLq0Y="), String.valueOf(d)).get()) == null || num2.intValue() != 0)) {
                        throw new IllegalStateException(m54.a("s1:575I3zaPxEtXOq1NpTrV5JW63l1mdJxQrbxidoI5TbfVBYouHej5n5KLPXkil4d6R1eHM5LUTcA7Fn9VzJksrOU8vs1JQ11mU8SZ8w==").toString());
                    }
                    if (d2 != 0.0d && ((num = (Integer) player.v(m54.a("s1:o8k7DLFUyM0w3cO83dkxUfvpznJ1PzpyMggyEVD/r8p+q6Z0oKIGSI1DeAJrw/DlHIw="), String.valueOf(d2)).get()) == null || num.intValue() != 0)) {
                        throw new IllegalStateException(m54.a("s1:qVDVCPZy4NN6Z2Ie4cXHJBrT6rIxD/UNJFKT9w+FxHpo3yoQkSm11G2hnWK277jc9zjZB3X91Yc/7s/l2OiCdDC+6tb8Qys/H8o=").toString());
                    }
                    if (up3Var3.U0.get() != i || up3Var3.c1.get()) {
                        throw new IllegalStateException(m54.a("s1:klMqCdRjosoRl8lxe/FVvBf3lcAGL3WYwDtVd+tuN64PEZMXThr0uDkZsS0wY3N51GISsEl36e0=").toString());
                    }
                    long jElapsedRealtime2 = SystemClock.elapsedRealtime();
                    boolean z5 = up3Var3.p && !fm0.K0(m54.a("s1:zO6eA4n2JhXdRxEadObv77s8gC8RXayHwxlcOaG/FrdagX8w56QYJw=="), m54.a("s1:uxoiB9VNRp2MNjutKTX0ZWomHy5mcj6e1PeAhWSxe+8i+Rd5652NdnmvxZk=")).contains(Build.HARDWARE);
                    kh3 kh3Var4 = up3Var3.C;
                    if (kh3Var4 == null) {
                        throw new IllegalStateException(m54.a("s1:vt4o3Zjf9BnrLNsOBVeZwcLDIKoTkuAgmTv4biDhLxsfXmnwF7o1tfN7Dh2jydGgavX08lB7dwdsyBXJtC591zT8BcYB+Da7jGU8Qw==").toString());
                    }
                    Player player3 = player;
                    up3Var3.t1 = kh3Var4.a(player3, wh3Var, z5, tp3VarP0.a(), tp3VarP0.e, z4 ? new tm1(j, nh3.g) : tp3VarP0.f, d3);
                    if (up3Var3.U0.get() != i) {
                        throw new IllegalStateException(m54.a("s1:klMqCdRjosoRl8lxe/FVvBf3lcAGL3WYwDtVd+tuN64PEZMXThr0uDkZsS0wY3N51GISsEl36e0=").toString());
                    }
                    up3 up3Var4 = up3Var3;
                    try {
                        n0(player3, z, i4, i5, up3Var4, i);
                        qi qiVar = qi.a;
                        qi.a(m54.a("s1:Zvsu+xA5iI2QgMKALj6VLvN2+x+3StKJZLZKYyrnoCZ2WGND3a95d+qY"), m54.a("s1:5ArLfNEAeqqewP63sb83VkHMzDK7bLw+xgW2bTlfxHIEXAHJsnsiNLjnJ2ReR/L0h5oioNTR+BGiZnV4ZHzzWXJM6f0GcniQC3ozhJzJnVXdvaRqjM6Z3K6mxNUUVLV32M4=") + (jElapsedRealtime2 - jElapsedRealtime) + m54.a("s1:TbLiLihpp23WaYd8w10+2ZVAg+Rzu5+/GxRkVm+Pwrlmej5c+o33bJZA/MFu3QUGUBHUWQSx") + (SystemClock.elapsedRealtime() - jElapsedRealtime2));
                        if (z4) {
                            sh3 sh3Var = (sh3) player3.t().get();
                            if (sh3Var == null || (arrayList = sh3Var.b) == null) {
                                lValueOf = null;
                            } else {
                                int size = arrayList.size();
                                int i8 = 0;
                                do {
                                    if (i8 >= size) {
                                        obj = null;
                                        break;
                                    }
                                    obj = arrayList.get(i8);
                                    i8++;
                                    ai3 ai3Var = (ai3) obj;
                                    if (gy1.n(ai3Var.f, m54.a("s1:MI/KfCDPxwtR0UsTnxJV8ADHvsD+y/XljwzKRMtcxtLPXw==")) && ai3Var.l) {
                                        Integer num3 = ai3Var.c;
                                        if ((num3 != null ? num3.intValue() : 0) == 0) {
                                            z3 = true;
                                        } else {
                                            z3 = false;
                                        }
                                    } else {
                                        z3 = false;
                                    }
                                } while (!z3);
                                ai3 ai3Var2 = (ai3) obj;
                                if (ai3Var2 != null) {
                                    lValueOf = Long.valueOf(ai3Var2.a);
                                } else {
                                    lValueOf = null;
                                }
                            }
                            d dVarC = tp3VarP0.c();
                            List list = sh3Var != null ? sh3Var.b : null;
                            if (list == null) {
                                list = sz0.g;
                            }
                            JSONArray jSONArrayS = gi3.S(list, tp3VarP0.b());
                            vh3 vh3Var = vh3.p;
                            Integer num4 = (Integer) player3.D(gi3.P(dVarC, jSONArrayS, vh3Var, lValueOf), vh3Var, up3Var3.t1).get();
                            if (!(num4 != null && num4.intValue() == 0)) {
                                throw new IllegalStateException(m54.a("s1:w4GBVJxnY+8rPSJJoVCIi291IXcMUZ8nRN8UvXGtYTcn1+PsvSB/JlxQheSQle1DDJfXIWBWxhXayTXtss5YymhP0zQM4PnnTl5YHnLj09maVdCb52U=").toString());
                            }
                        }
                        up3 up3Var5 = up3Var3;
                        try {
                            o0(player3, up3Var5, i, zv4Var, jSONObject2, jSONObject3, tp3VarP0);
                            up3Var2 = up3Var5;
                            up3Var2.d0 = jElapsedRealtime2;
                            up3Var2.c0 = true;
                            up3Var2.b0 = null;
                            i6 = i;
                        } catch (Exception e3) {
                            e = e3;
                            up3Var3 = up3Var5;
                            i7 = i;
                            up3Var3.X();
                            up3Var3.q(i7, e);
                            return;
                        } catch (LinkageError e4) {
                            e = e4;
                            up3Var3 = up3Var5;
                            i7 = i;
                            up3Var3.q(i7, e);
                            return;
                        }
                    } catch (Exception e5) {
                        e = e5;
                        up3Var3 = up3Var4;
                    } catch (LinkageError e6) {
                        e = e6;
                        up3Var3 = up3Var4;
                    }
                } else {
                    Object obj3 = player.u(wh3Var).get();
                    m54.a("s1:RL7W/MtdNGP866czqj9qUvW1BZgYMrJ0EGwcUs/0bl9ypfPG1dDSzdGH5So=");
                    obj3.getClass();
                    long jLongValue = ((Number) obj3).longValue();
                    up3Var3.t1 = jLongValue;
                    if (!(jLongValue != 0)) {
                        throw new IllegalStateException(m54.a("s1:goqaGKVS19V/oMaDi8uBZGR/OazciC0BUyPnF1gr1f5UYcXZK0HYAaewBW3mEt1ISVGV2ds9"));
                    }
                    Player player4 = player;
                    Integer num5 = (Integer) player4.G(m54.a("s1:/abxt5V/PuVJ9XQe0mkALINIujB70NLaYF7/0mamNvlaVpEU"), jLongValue, new String[0], d3).get();
                    if (!(num5 != null && num5.intValue() == 0)) {
                        throw new IllegalStateException(m54.a("s1:JGUtpcsBbKk0P8qNPn5ZxsOAUhuioiSizHZl0by3KAOZ28Vm/UC3d5u6zsM9HmstJFWk2wh+T1XGsalz3Z4dsY9APQM1Z8gW").toString());
                    }
                    if (!(d == 1.0d)) {
                        player4.v(m54.a("s1:EszcE/g81WSB/nKWCB08NUqe+6i3Aay8Q4xE9vWi+dtNYPOLq0Y="), String.valueOf(d)).get();
                    }
                    if (!(d2 == 0.0d)) {
                        player4.v(m54.a("s1:o8k7DLFUyM0w3cO83dkxUfvpznJ1PzpyMggyEVD/r8p+q6Z0oKIGSI1DeAJrw/DlHIw="), String.valueOf(d2)).get();
                    }
                    up3 up3Var6 = up3Var3;
                    try {
                        up3Var2 = up3Var6;
                        i6 = i;
                        up3Var2.b0 = new kp3(player4, up3Var6, i, linkedHashMap, str4, jSONObject, jSONObject2, jSONObject3, jSONObject4, zv4Var);
                    } catch (Exception e7) {
                        e = e7;
                        up3Var3 = up3Var6;
                        i7 = i;
                        up3Var3.X();
                        up3Var3.q(i7, e);
                        return;
                    } catch (LinkageError e8) {
                        e = e8;
                        up3Var3 = up3Var6;
                        i7 = i;
                        up3Var3.q(i7, e);
                        return;
                    }
                }
                up3Var2.v1 = 0;
                up3Var2.j(i6, new yj3(up3Var2, 10));
            } catch (Exception e9) {
                e = e9;
            } catch (LinkageError e10) {
                e = e10;
            }
        } catch (Exception e11) {
            e = e11;
        } catch (LinkageError e12) {
            e = e12;
        }
    }

    public static /* synthetic */ void n(up3 up3Var, kf1 kf1Var, yj3 yj3Var, mf1 mf1Var, int i) {
        int i2 = 24;
        if ((i & 1) != 0) {
            kf1Var = new c5(i2);
        }
        kf1 c5Var = yj3Var;
        if ((i & 2) != 0) {
            c5Var = new c5(i2);
        }
        up3Var.m(kf1Var, c5Var, mf1Var);
    }

    public static final void n0(Player player, boolean z, int i, int i2, up3 up3Var, int i3) {
        Integer num = (Integer) player.v(m54.a("s1:IOfFB0HhyhneLITDSx+caH+gsnPhGCIXZqSpaWSsd3vIjwpc1/X94tbOBU5J7mHQFeycxyqG4lp7OjJQNQs99xAE3jGNVwlYwJ7DD4A3"), z ? m54.a("s1:HHLRJP+5KL+ca+61SN1SmHYc2wMUmY5fy7izaWReymleXw==") : "no").get();
        if (num == null || num.intValue() != 0) {
            kb2.j(m54.a("s1:EPtAMylhRAr4k14f89RpRITNG7c997y9JpnlQ1wNPpyb6DTIIn+a6OwxiWiCyFwpzDXV6HuFkSuDUSMmbicI0xIz2YkA3oHSzgT2Z3NfxWONzg17NZeHvg9QxyU="));
            return;
        }
        Integer num2 = (Integer) player.v(m54.a("s1:1J34cLYJhQz/HP+I1CXofvHKoPrBy++xYruPJqOW9AJZOeUXT2ua1O1y7d/sMJKlFYdzxYnfwM/3MKGPymk="), String.valueOf(i)).get();
        if (num2 == null || num2.intValue() != 0) {
            kb2.j(m54.a("s1:aObflMubUYbnQrXOJ9SkMiV9yGIApGXjOExemV/gueNBHkt8mGRDvqFiVKMHh/i6pL1asfI/jKdGxZU35dzs1MJHO51LTkH5lvElk8T5ynbrpYzr"));
            return;
        }
        Integer num3 = (Integer) player.v(m54.a("s1:pcQidifEkwxnOVtGz3C8Si1mBA3iCxRLzUfS/Z98DcdC3t2+jdm6LNfg+rkQfW5x7od7ECyH6Y08LQ=="), String.valueOf(i2)).get();
        if (num3 != null && num3.intValue() == 0) {
            return;
        }
        up3Var.j(i3, new yj3(up3Var, 11));
        qi qiVar = qi.a;
        qi.e(m54.a("s1:AodeLleWIm9585WmCvU47aiQ87T65WydYYN+lIMLNJwmqNZ94FofFJk7UuI="), m54.a("s1:Kwj6537RQ57uRaIS8QcWdR5qyhWzxY11yH/p9Oe9xl6tAA+Cfd2tpJIqNgKrxLtqqdhWCLtdG+q2iZbi7zKYJ8bdcuJzzIVjRg/QcUpy"));
    }

    public static final void o0(Player player, up3 up3Var, int i, zv4 zv4Var, JSONObject jSONObject, JSONObject jSONObject2, tp3 tp3Var) throws JSONException {
        boolean z;
        Integer num;
        Map map = tp3Var.a;
        sh3 sh3Var = (sh3) player.t().get();
        if (up3Var.U0.get() == i && sh3Var != null) {
            ArrayList arrayList = sh3Var.b;
            if (sh3Var.a == up3Var.t1) {
                List list = sz0.g;
                List list2 = arrayList == null ? list : arrayList;
                LinkedHashMap linkedHashMap = new LinkedHashMap(pj2.S(map.size()));
                for (Map.Entry entry : map.entrySet()) {
                    Object key = entry.getKey();
                    String str = (String) entry.getValue();
                    String str2 = (String) up3Var.W.get(str);
                    if (str2 != null) {
                        str = str2;
                    }
                    linkedHashMap.put(key, str);
                }
                up3Var.P(gi3.S(list2, linkedHashMap), null, null, null, false);
                d dVar = tp3Var.b;
                String strA = m54.a("s1:dI8VM1k+Tt7EpvdrLMzFG5IK9vbp3g6lXNpm/cB2qOEfcrFvc8s+H4ScuJelIQ==");
                JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:moJWpINXfzQZjGiePwZQZ/2NF3AerPIgSzUvVNLReobnfhvWzkvnZQ=="), up3Var.t1);
                String strA2 = m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                if (arrayList != null) {
                    list = arrayList;
                }
                dVar.f(strA, jSONObjectPut.put(strA2, gi3.E(list, map)));
                if (arrayList == null || arrayList.isEmpty()) {
                    z = false;
                } else {
                    int size = arrayList.size();
                    int i2 = 0;
                    while (i2 < size) {
                        Object obj = arrayList.get(i2);
                        i2++;
                        ai3 ai3Var = (ai3) obj;
                        if (gy1.n(ai3Var.f, m54.a("s1:MI/KfCDPxwtR0UsTnxJV8ADHvsD+y/XljwzKRMtcxtLPXw==")) && ai3Var.l && (num = ai3Var.c) != null && num.intValue() == 1) {
                            z = true;
                        }
                    }
                    z = false;
                }
                Integer num2 = (Integer) player.M(zv4Var.c(z), up3Var.t1).get();
                up3Var.j(i, new pi0(up3Var, tp3Var, sh3Var, jSONObject, jSONObject2, num2, 6));
                qi qiVar = qi.a;
                String strA3 = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                long j = up3Var.t1;
                boolean z2 = zv4Var.d;
                StringBuilder sb = new StringBuilder();
                y82.s("s1:6VUFTD2xp9VRLw5AORa4hPuMlxSmHkrTKAQI0CHOFJuX03LZOU5x4Ibk/cS4IcVmGwI=", sb, j, "s1:sLUV3WEGfHcsaXKybokFs6p/aQoDIKa7R0HJ9p1KfOJG43C0Uhy460T9qGHyIZ52iBo=");
                sb.append(z);
                sb.append(m54.a("s1:wrQn5d6P7wvQ7/oukko5jKiZAMdc8jqRICd5XHrkGPuMecyB+tUAk3auUo8="));
                sb.append(z2);
                sb.append(m54.a("s1:nsuLaxvfp0hioEg5D/c/df+h2qb7HptaP+zCbGpQnhI3FqWs49MmmIueQNY="));
                sb.append(num2);
                qi.a(strA3, sb.toString());
                return;
            }
        }
        kb2.j(m54.a("s1:klMqCdRjosoRl8lxe/FVvBf3lcAGL3WYwDtVd+tuN64PEZMXThr0uDkZsS0wY3N51GISsEl36e0="));
    }

    /* JADX WARN: Code duplicated, block: B:72:0x01df  */
    public static final tp3 p0(LinkedHashMap linkedHashMap, Player player, up3 up3Var, String str, JSONObject jSONObject, JSONObject jSONObject2, JSONObject jSONObject3, JSONObject jSONObject4, int i, boolean z) throws ExecutionException, JSONException, InterruptedException {
        JSONObject jSONObject5;
        JSONObject jSONObject6;
        tm1 tm1Var;
        long j;
        sh3 sh3Var;
        String str2;
        JSONObject jSONObject7;
        ArrayList arrayList = new ArrayList();
        Iterator it = linkedHashMap.entrySet().iterator();
        while (true) {
            ib3 ib3Var = null;
            if (!it.hasNext()) {
                break;
            }
            Map.Entry entry = (Map.Entry) it.next();
            String str3 = (String) entry.getKey();
            String str4 = (String) entry.getValue();
            if (new File(str3).isFile()) {
                Object obj = player.e(m54.a("s1:lkxmtULMMKPWn+kE7EqNyaf5Oo4KuooZ5rSIIgwf0z1YvNjlOUqGICeDAKuS+AtSCFM="), up3Var.t1, new String[]{str3, str4}, new double[0]).thenApply((Function) new ev2(25)).get();
                if (((Long) obj).longValue() <= 0) {
                    obj = null;
                }
                Long l = (Long) obj;
                if (l != null) {
                    ib3Var = new ib3(Long.valueOf(l.longValue()), str3);
                }
            }
            if (ib3Var != null) {
                arrayList.add(ib3Var);
            }
        }
        Map mapY = pj2.Y(arrayList);
        sh3 sh3Var2 = (sh3) player.t().get();
        List list = sh3Var2 != null ? sh3Var2.b : null;
        if (list == null) {
            list = sz0.g;
        }
        JSONArray jSONArrayS = gi3.S(list, mapY);
        try {
            String strV0 = up3Var.v0(str);
            if (up3Var.x == null || up3Var.y == null || strV0.length() <= 0 || up3Var.v == null || !((Boolean) up3Var.B.a()).booleanValue()) {
                jSONObject5 = null;
            } else {
                gg3 gg3Var = up3Var.v;
                JSONObject jSONObject8 = up3Var.x;
                JSONObject jSONObject9 = up3Var.y;
                hi3 hi3Var = up3Var.o;
                String str5 = hi3Var.k;
                String str6 = hi3Var.l;
                JSONObject jSONObject10 = up3Var.A;
                synchronized (up3Var.w) {
                    jSONObject7 = new JSONObject(up3Var.w.toString());
                }
                jSONObject5 = xf3.g(up3Var.y, strV0, strV0, gg3Var.a(jSONObject8, jSONObject9, strV0, str5, str6, jSONObject10, jSONObject7), jSONArrayS);
            }
        } catch (Exception e) {
            qi qiVar = qi.a;
            y82.t("s1:Pg3+vR5kHyk8/BrKR5InBcf+w6jQRccgYFykF73V4UoWd8G2FH2DZOf2syaWFR6F0vugo97HmLnFwsYHSSEeb6f/q22XqvClGtRVBL6bUt+wdPn65GdeEWdcSX4=", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:gwLuLUmwmaMxvWwQJvCFsaRW/5R70HpWdo0c/jdHWbkuDxfUqXl4wHhtdGgy4OUdSgvHd6nxxZk="));
        }
        hi3 hi3Var2 = up3Var.o;
        d dVar = new d(hi3Var2.k, hi3Var2.l);
        dVar.f(m54.a("s1:kVR+9kcfBYK7eD3GfTgbCLx5/t51KDI4WUzlQG5AmN8gk9rrgkT5zLhv"), new JSONObject().put(m54.a("s1:wkAszeAyk9keyL3uHMHQXBJFuF71xZG6rvo5c+P68CS6Xzb4gHQneA=="), new JSONObject().put(m54.a("s1:9vE/KjVA3hwcr9gEI5UbEtl78jVONDicVQw24DjmDd7xdjBJvLI="), jSONObject == null ? JSONObject.NULL : jSONObject).put(m54.a("s1:Y+IrCff8CTSYNqNe0Dgq4gIvM4GPVAUeq9ZXx31lzJOcdXpx8I7Y938kAVo="), jSONObject2 == null ? JSONObject.NULL : jSONObject2).put(m54.a("s1:/bcqGsQMlUwQJmpCAectwP6CtNea/85YbqojUdDH0aEOlUnlT6+mBlNEvrnCFA=="), jSONObject3 == null ? JSONObject.NULL : jSONObject3)));
        String strA = m54.a("s1:XV4UMJlWgu2wspJK2zotZudXy28dD+X2CX3X/U1ejVZZzWAU9SZJacBu");
        JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:moJWpINXfzQZjGiePwZQZ/2NF3AerPIgSzUvVNLReobnfhvWzkvnZQ=="), up3Var.t1);
        String strA2 = m54.a("s1:TeOk/AkblTgIc9dLno1sourA5w00FCbJ6RfnkeaAPmHWRH/zxHZiWg==");
        if (jSONObject5 == null) {
            jSONObject5 = new JSONObject();
        }
        JSONObject jSONObjectF = dVar.f(strA, jSONObjectPut.put(strA2, jSONObject5).put(m54.a("s1:1Egp5AjuSpuwslsTuHn7G7Q5sl+4qIqMF3fyjbWaefgq2HCZ"), up3Var.u == null).put(m54.a("s1:TQBhIWAptLfnWD5eWuR7SVxvZv8n8RX22o1V1h2gk86JpfjDAmP8iQwhHDaif8EARVuGwg=="), up3Var.A));
        JSONObject jSONObject11 = up3Var.x;
        if (jSONObject11 == null) {
            jSONObject6 = new JSONObject();
        } else {
            gg3 gg3Var2 = up3Var.v;
            jSONObject6 = gg3Var2 != null ? gg3Var2.g(jSONObject11.getJSONObject(m54.a("s1:NmOEhG0XOOWBSzuoMholvdu97FZ8jWLS5Zaqi0Y1TTjle+k5vSq3I57EkVju5w==")).getString(m54.a("s1:wCWlmCtR9Y6e7urINUeKAVOk4W9mwoq8iJwCpOxvM6Ji4hY6dk2VX6tOWXY=")), jSONObject11.optString(m54.a("s1:O+p7ysgRXuVp/mu9vc0pyNC2su413e1rvx5KReq5MLxHwKRXm1aWeWieqDA="))) : null;
            if (jSONObject6 == null) {
                jSONObject6 = new JSONObject();
            }
        }
        synchronized (up3Var.w) {
            Iterator<String> itKeys = up3Var.w.keys();
            m54.a("s1:KEIV5HJJs4nkN8qY+AOytsCyUlwamYKbpJY6sS6tvJ8AP3pT+LZkA6wG65JC4Q==");
            itKeys.getClass();
            while (itKeys.hasNext()) {
                String next = itKeys.next();
                jSONObject6.put(next, up3Var.w.get(next));
            }
        }
        if (up3Var.u == null) {
            jSONObjectF = dVar.f(m54.a("s1:0AioOnVKZNGh2YMDdBi4kQ8gMbqt4gVHywKF9FNbiBxe4Pcj+7oMPsr8BXVPIKNk"), new JSONObject().put(m54.a("s1:Zuols6/cGe4ETNBAdwTQ+nq6FPTbQZ58dfbDsZ02+TblatsG2mW/WHc37rFhMbr80j4="), up3Var.o.n).put(m54.a("s1:j5aWvvI0goIAzm9kXPmwDxVdrLJtvtK4qc/aU2EJ2URrp/5Js84SS91XmBfGhPsX0FVvIN5bYhI="), up3Var.o.o).put(m54.a("s1:wqTLDVJTgoKQ7bkXAhTrGCXuxgRegG/Lx1Zy3ncncS4wvOtIB0E1zg=="), jSONObject6).put(m54.a("s1:wkAszeAyk9keyL3uHMHQXBJFuF71xZG6rvo5c+P68CS6Xzb4gHQneA=="), new JSONObject().put(m54.a("s1:6V4x8+QHdLG26I1bkxNtLjL86qA4Fm0LAFzjiJYFyOJP0eJC3Q2NtBSp"), jSONObject4.optString(m54.a("s1:G8igenSZk44xDQqzHi+SHzrgl8N11SFlwgu5m2Zo4gxcbVV3F2CBk+ODamC4Mq59zcKYiQ=="))).put(m54.a("s1:KzEO2UsuNlooR2UJH4MDtD0e6mpXwVIePT+wPwG0czceXQadlnLVQDKpqoiLMdks"), jSONObject4.optString(m54.a("s1:GULWr3YrEP4zF8BDQ/pfw/E/IQDaD+a50nUHdkuR2xj4uggIBkB0k+FcYSB0hADBZBCfjM89ymRTXpXc/8M="))).put(m54.a("s1:h0maXTNlucaZ0Ks2ploC5UwYkySnU5gjJrBrWw8abrZfI+OpGQTSCbG45XCyNPH4jEM="), jSONObject4.optString(m54.a("s1:u99CXtoTgRaaHNfPYvVUHMuvu5PZoJO8vL8zeczWc6MKjazLwS72y883G0fE2yrufmHp/YqqsW3DN1YHnhwP3YMG")))));
        }
        LinkedHashMap linkedHashMap2 = new LinkedHashMap();
        JSONObject jSONObject12 = jSONObjectF.getJSONObject(m54.a("s1:P1KLi0q82xJSDbiKqbljrKDW2hfYoAK7PFxexU+wlscM9Jhb+G+Egdlo"));
        for (vh3 vh3Var : fm0.K0(vh3.o, vh3.p)) {
            m54.a("s1:9APGw38pIRmFFr7xugW++AnUv7+waqJhCCY5sZBpeo/SFsH7");
            vh3Var.getClass();
            int iOrdinal = vh3Var.ordinal();
            if (iOrdinal == 0) {
                str2 = "s1:9vE/KjVA3hwcr9gEI5UbEtl78jVONDicVQw24DjmDd7xdjBJvLI=";
            } else if (iOrdinal == 1) {
                str2 = "s1:Y+IrCff8CTSYNqNe0Dgq4gIvM4GPVAUeq9ZXx31lzJOcdXpx8I7Y938kAVo=";
            } else {
                if (iOrdinal != 2) {
                    vx0.i();
                    return null;
                }
                str2 = "s1:/bcqGsQMlUwQJmpCAectwP6CtNea/85YbqojUdDH0aEOlUnlT6+mBlNEvrnCFA==";
            }
            String strOptString = jSONObject12.optString(m54.a(str2));
            strOptString.getClass();
            if (strOptString.length() <= 0) {
                strOptString = null;
            }
            if (strOptString != null) {
                linkedHashMap2.put(vh3Var, strOptString);
            }
        }
        if (z) {
            tm1Var = gi3.P(dVar, jSONArrayS, vh3.g, null);
            j = 0;
        } else {
            j = 0;
            tm1Var = new tm1(j, nh3.o);
        }
        tm1 tm1VarP = z ? gi3.P(dVar, jSONArrayS, vh3.o, null) : new tm1(j, nh3.o);
        tm1 tm1Var2 = ((nh3) tm1VarP.o) == nh3.p ? tm1VarP : null;
        tm1 tm1VarP2 = z ? gi3.P(dVar, jSONArrayS, vh3.p, tm1Var2 != null ? Long.valueOf(tm1Var2.g) : null) : new tm1(0L, nh3.g);
        if (up3Var.U0.get() == i && (sh3Var = (sh3) player.t().get()) != null && sh3Var.a == up3Var.t1) {
            return new tp3(mapY, dVar, linkedHashMap2, tm1Var, tm1VarP, tm1VarP2);
        }
        kb2.j(m54.a("s1:klMqCdRjosoRl8lxe/FVvBf3lcAGL3WYwDtVd+tuN64PEZMXThr0uDkZsS0wY3N51GISsEl36e0="));
        return null;
    }

    public static void s(up3 up3Var, boolean z, boolean z2, boolean z3, int i) {
        pj3 pj3Var;
        boolean z4 = (i & 2) != 0 ? false : z2;
        int i2 = 4;
        boolean z5 = (i & 4) != 0 ? false : z3;
        boolean z6 = up3Var.b1;
        up3Var.b1 = z && z5;
        if (!z && (pj3Var = up3Var.I) != null) {
            pj3Var.m(true);
            for (hj3 hj3Var : pj3Var.g) {
                pj3.b(hj3Var);
            }
            pj3Var.i(false);
        }
        if (z && z4 && !up3Var.K().f()) {
            if (((Boolean) up3Var.h.a()).booleanValue()) {
                up3Var.q1 = false;
                qi qiVar = qi.a;
                qi.a(m54.a("s1:AodeLleWIm9585WmCvU47aiQ87T65WydYYN+lIMLNJwmqNZ94FofFJk7UuI="), m54.a("s1:9fpPtqIIfokHK5O2VtH2haBghXlS4Ktm0KLbIqzStUKBEo+IYA5Hwq+vEohprpo2UXDoLUBEv5fwPM0atuC3pdsyS0JmccidGVqCyW3xN/I="));
            } else {
                up3Var.i0(u9.V(m54.a("s1:+xRu3SiQ16DwkL3N3I2gBOmUWNq2JYeimRcJBVxiMbDcSbhFe3huAvZ0OWNjQwC2wrWBDNPbiVWN9imJ0QffbtyETZ+dvrElsAt3BHlrAfXCEzI/a9qD+dVVxXqr4PU5XA3XTC3n2Iwg5ihn5dw="), new Object[0]), false);
            }
        }
        int i3 = 3;
        if (up3Var.o1 != z) {
            up3Var.o1 = z;
            if (z) {
                if (up3Var.e1 != null || up3Var.b1) {
                    up3Var.k0();
                    return;
                }
                return;
            }
            up3Var.l();
            up3Var.q1 = true;
            up3Var.g0(zp3.a(up3Var.K(), null, 0.0d, 0.0d, null, true, 0, null, 1073741759));
            n(up3Var, null, null, new wn3(i2), 3);
            up3Var.S();
            up3Var.i.a();
            return;
        }
        if (z && up3Var.b1) {
            if (up3Var.p1) {
                up3Var.S();
            } else {
                up3Var.k0();
            }
        } else if (z && z6) {
            up3Var.k0();
        }
        if (z && z4 && !up3Var.q1 && up3Var.K().k()) {
            n(up3Var, null, null, new wn3(i3), 3);
        }
    }

    public final int A() {
        return this.o.r;
    }

    public final double B() {
        return ((Number) this.q0.getValue()).doubleValue();
    }

    public final boolean C() {
        JSONArray jSONArrayOptJSONArray;
        JSONObject jSONObject = (JSONObject) this.n0.getValue();
        if (jSONObject != null && (jSONArrayOptJSONArray = jSONObject.optJSONArray(m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg=="))) != null) {
            xw1 xw1VarC = jg5.C(0, jSONArrayOptJSONArray.length());
            ArrayList arrayList = new ArrayList();
            Iterator it = xw1VarC.iterator();
            while (true) {
                ww1 ww1Var = (ww1) it;
                if (!ww1Var.p) {
                    break;
                }
                JSONObject jSONObjectOptJSONObject = jSONArrayOptJSONArray.optJSONObject(ww1Var.nextInt());
                if (jSONObjectOptJSONObject != null) {
                    arrayList.add(jSONObjectOptJSONObject);
                }
            }
            if (!arrayList.isEmpty()) {
                int size = arrayList.size();
                int i = 0;
                while (i < size) {
                    Object obj = arrayList.get(i);
                    i++;
                    JSONObject jSONObject2 = (JSONObject) obj;
                    if (gy1.n(jSONObject2.optString(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn")), m54.a("s1:ykA/aQnECxnqEbtKzLAqb3Qx+OcJaYRIwSzDB0dsGnn3byqD98U=")) && !jSONObject2.optBoolean(m54.a("s1:X0MAh/3pY/BiVsfLj67tazEpBXftqNn4ivcSjXMmi2ffM0skuJwH8PDQ6BQ="))) {
                        return true;
                    }
                }
            }
        } else if (K().i > 0 && K().j > 0) {
            return true;
        }
        return false;
    }

    public final JSONObject D() {
        return (JSONObject) this.F0.getValue();
    }

    public final String E() {
        return (String) this.p0.getValue();
    }

    public final long F() {
        return this.v0.g();
    }

    public final Double G() {
        return (Double) this.H0.getValue();
    }

    public final double H() {
        return ((Number) this.s0.getValue()).doubleValue();
    }

    public final fn3 I() {
        return this.z;
    }

    public final int J() {
        return ((Number) this.f1.getValue()).intValue();
    }

    public final zp3 K() {
        return (zp3) this.e0.getValue();
    }

    public final String L() {
        return this.c;
    }

    public final zv4 M() {
        return (zv4) this.L.getValue();
    }

    public final List N() {
        return (List) this.f0.getValue();
    }

    public final boolean O() {
        return ((Boolean) this.h1.getValue()).booleanValue();
    }

    public final void P(JSONArray jSONArray, String str, String str2, String str3, boolean z) {
        synchronized (this.w) {
            try {
                JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:ew40n2n3bxNGguvFgPaMhhToWkI3RPcXBsNDffYCWEeAr5ckMtqqtA=="), m54.a("s1:3j36Xyy7drmfsEUNwhgTefqbXTjwd0sdgZhjXFga6V7MsEo01AmO3uff3dIOZVqHpWqlTVh4yiOkP4t0C57YmHkj")).put(m54.a("s1:8aF7X9zhXLrnEipKNGch6DetrdRZCvXkqiy+7lXny5qednee024="), this.w).put(m54.a("s1:N258ecO02G8Q/tNhQPbSvppm18CydPUMC5K11qXBhBSOqL+8jbra05T0"), jSONArray);
                if (str != null) {
                    jSONObjectPut.put(m54.a("s1:2VArilRjwyLHtJn6SZEfQP6jYY4IOMgUoruAm5ElSLBmS2I0"), str).put(m54.a("s1:jFX7dlAmG/I7yKN9HKXQBBBQnjuAXKKnNCOrIdig2GxJ4R5mgNcIbim8Srg="), str2).put(m54.a("s1:K70aJgcChvGrqpuf9yA2pxFcl3mR7wou9H4sErTAzgVQIS5w"), m54.a(gy1.n(str2, "-1") ? "s1:YijQ907msR/0eaJD6HxhroI97jqrRs2MVWqVMy0gyHj5kA==" : "s1:PiyeVCuVuDsBnVmhSiq0tCGpauhD7u5AHQbwAi20rwUTTSmUXvs=")).put(m54.a("s1:wCWlmCtR9Y6e7urINUeKAVOk4W9mwoq8iJwCpOxvM6Ji4hY6dk2VX6tOWXY="), str3).put(m54.a("s1:CPneE9cBEecqfJd9HCVwT/UZ3saO0LvakiW96mOs5N9J43T53jVIfJVSeTWXHOefFDV+EzARI/U="), z);
                }
                Object objN = fa1.n(m54.a("s1:7u1skT13NtFlsj6uNvSMGZ+DbcPTuiGR4CddEp06wL9Oua0qP7vbDTAgGX59PHOb1aE="), jSONObjectPut);
                m54.a("s1:1/lFr6r4FbK6DiAQncJeiDPDsJB5+VgpiDbGfXpcgp9De522lWoW6jLE8XV1S+jON7YHcqU12Y45faI0eqvgkJr7vSlVQU4gvhwhuA1UDOwGMAoMruBCTmlpTa14A0Q7pOCNwMCXzn2VxILYtDgFFWEWuYMJnp5HSO37LXCVK384D2tu5yzDcusbDzE=");
                objN.getClass();
                JSONObject jSONObject = (JSONObject) objN;
                Iterator<String> itKeys = jSONObject.keys();
                m54.a("s1:KEIV5HJJs4nkN8qY+AOytsCyUlwamYKbpJY6sS6tvJ8AP3pT+LZkA6wG65JC4Q==");
                itKeys.getClass();
                while (itKeys.hasNext()) {
                    String next = itKeys.next();
                    this.w.put(next, jSONObject.get(next));
                }
            } catch (Throwable th) {
                throw th;
            }
        }
    }

    public final JSONObject Q(boolean z, boolean z2) throws JSONException {
        return y82.o("s1:vDloJ0USn7M0YgPKK45msa8H4YPjQbf5PSUHBIJxYrmj8bPE", "s1:08h3zfqQF/l2d6BTFYhTqHR7dUtuve5E9ob4IClNZdLQB33MuFPrjPDybTdkwyBshNKWYHXMfNufCYzomRJfyA==", this.E0.v(m54.a("s1:RedN1UjFxHJbkQgcIf585mvAnxWascq8LBfNF1URbJG/jiehlbdmQKi0"), new JSONObject().put(m54.a("s1:Q95/7cZorYq8vCixrcbxHT4ot4ZvtMMP7vjL07lEuKvXdA=="), this.a).put(m54.a("s1:Nv5EgEdihDhTiddue6X9OVfQqLyoFvjoxZcffujD4vHWVmbOcaY="), K().b).put(m54.a("s1:chk211alLfsdDStPMaYQVD+skqo1kpZ4Oevyg/W/z3AgXd68DtXG9A=="), K().g).put(m54.a("s1:GmCjJnVg2jleqADdDiHFvbvz9Ih9WaaY1jj6eNCL79d60qdw0ugT6Gqhfkf0Yg=="), z).put(m54.a("s1:EaMD80uwwzJLNeMeaiQMjspT7u2hviL/FOQuoL3p6U0xpyVr3qMs60BP"), this.V0).put(m54.a("s1:pqS2BVxRkwnmuPnA1IHrIt2cnkHHkb28fTXAj/aZmxGW1g=="), z2)));
    }

    public final boolean R() {
        String lowerCase = null;
        List listK0 = fm0.K0(null, m54.a("s1:1Egp5AjuSpuwslsTuHn7G7Q5sl+4qIqMF3fyjbWaefgq2HCZ"), "fd", m54.a("s1:IWDsK99eBDlfSljTCweHK83amr65m2/FNOAKKH/jhB67MzzcAz5QGjCl"), m54.a("s1:TOsAepQlUK17mU+ittyWPm9WiRdBevXQpICQM0GvNwQiihSIvB5Igg166cfpFuE1Qbk/8HjyzwZ+LAIE"));
        String scheme = Uri.parse(this.a).getScheme();
        if (scheme != null) {
            lowerCase = scheme.toLowerCase(Locale.ROOT);
            m54.a("s1:IMkR/46TPcyN8H41FEDRPx1tnyp9fXfX8O1+o+0Hknr6ftryKWxG64PxNxbvAX+f40Hjs04Fvg+rVjOK");
            lowerCase.getClass();
        }
        return listK0.contains(lowerCase);
    }

    public final void S() {
        if (this.V0 || this.c1.get() || !this.p1 || this.a1) {
            return;
        }
        vk3 vk3Var = this.Z0;
        if (vk3Var == null) {
            vk3Var = new vk3();
            this.Z0 = vk3Var;
        }
        this.a1 = true;
        qi qiVar = qi.a;
        qi.a(m54.a("s1:AodeLleWIm9585WmCvU47aiQ87T65WydYYN+lIMLNJwmqNZ94FofFJk7UuI="), m54.a("s1:oJ1cf+e2aUCUd02C3HfBBn070EqZW+jWhNhMV8lFoic4c3EjAsZOQXvLMqHS3Xz435TZMgN9KO9noMNqbJ5Rc9hGK6MSM2kPIO0oy5iDqCTy3HmMV0F4LdPujk0Sug==") + K().p);
        n(this, null, new yj3(this, 6), new hm1(25, vk3Var), 1);
    }

    public final void T() {
        this.q1 = true;
        n(this, null, null, new wn3(5), 3);
    }

    public final void U() {
        if (!this.W0) {
            qi qiVar = qi.a;
            qi.a(m54.a("s1:AodeLleWIm9585WmCvU47aiQ87T65WydYYN+lIMLNJwmqNZ94FofFJk7UuI="), m54.a("s1:5qRTuF+HlPuHx1ekPJBAoCCYNNV/pFQQWluD1mqbprVTqyvyFD3Ub8vY4Q5mveH49EkkIdPP8rY1wSVG1JDqrV3lXPhJPJQgt2rWf4QD4Xw7lvnlbx1OFfxTzAmWeq6Y") + K().a);
        }
        this.W0 = true;
        this.R0.removeCallbacks(this.x1);
        l();
        this.T.e();
        xb3 xb3Var = this.X;
        tz0 tz0Var = tz0.g;
        xb3Var.setValue(tz0Var);
        this.Y = tz0Var;
        Boolean bool = Boolean.FALSE;
        this.i0.setValue(bool);
        this.t0.setValue(null);
        this.u0.setValue(bool);
        e(true);
    }

    public final void V() {
        Surface surface;
        if (this.V0 || this.c1.get() || !this.b1) {
            return;
        }
        Surface surface2 = this.e1;
        if (surface2 == null || !surface2.isValid() || (surface = this.l1) == null || !surface.isValid()) {
            vk3 vk3Var = this.Z0;
            if (vk3Var == null) {
                vk3Var = new vk3();
                this.Z0 = vk3Var;
            }
            this.e1 = vk3Var.o;
            this.l1 = vk3Var.p;
            this.m1 = 16;
            this.n1 = 16;
            this.k1 = null;
            this.j1.setValue(Boolean.FALSE);
            qi qiVar = qi.a;
            qi.a(m54.a("s1:AodeLleWIm9585WmCvU47aiQ87T65WydYYN+lIMLNJwmqNZ94FofFJk7UuI="), m54.a("s1:nCaaBbbNlfMznYWQxZTHnqDqgJfA02+8dfUP4wnrmZjzjlIoFdHLHKCOVKZ7L6EjcZsHaSeys1X3ldrDL8yHLVJFb3bm4p0N/GTSs3CKNWgS848X2nbElQUImnIYwA=="));
        }
    }

    public final void W(double d) throws Exception {
        boolean zE;
        Double dValueOf;
        eg3 eg3Var;
        if (this.V0 || !K().k() || !K().h || Math.abs(d) > Double.MAX_VALUE || K().i <= 0) {
            return;
        }
        double dF = jg5.f(d, 0.0d, K().d);
        try {
            zE = n86.e(dF / K().d, K().f, K().a > 0 && K().k() && !this.c1.get() && ((eg3Var = this.z1) == null || !eg3Var.c()), R());
            if (this.J0) {
                qi qiVar = qi.a;
                qi.a(m54.a("s1:hG6dzE1TatVbxTmCddXSZ8/rHV8RWy1vJ1/RjbjObmPy3DDZUSJz3NTJiTk="), m54.a("s1:IbMVAu6EEx0gs8XawR2/Vml6bLDc6mdukM3Kntvv6ES1jNIx8TiTMHNGAiB9qzcnfvK5tME5pc8CvuK72pCGt5HpeoLhNLMyew/PMOuVQHM="));
            }
            this.J0 = false;
        } catch (Exception e) {
            if (!(e instanceof IOException) && !(e instanceof JSONException)) {
                throw e;
            }
            if (!this.J0) {
                qi qiVar2 = qi.a;
                y82.t("s1:mcKiKSV/esO6dWiKmxvkqCK9j3+FRClVQxjjl2ogJ7HjEOUIZ59p1oES7xYmq1MqhKT+Axf//hTB7V32vo68GzkP23ju7XZJzf7UBngfj1sC2rzoSb4=", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:hG6dzE1TatVbxTmCddXSZ8/rHV8RWy1vJ1/RjbjObmPy3DDZUSJz3NTJiTk="));
            }
            this.J0 = true;
            zE = false;
        }
        if (!zE) {
            f();
            return;
        }
        if (G() == null) {
            this.M0 = false;
            this.N0 = SystemClock.elapsedRealtime();
            this.O0 = 0;
            this.P0 = 0;
            this.Q0 = 0;
            qi qiVar3 = qi.a;
            qi.a(m54.a("s1:hG6dzE1TatVbxTmCddXSZ8/rHV8RWy1vJ1/RjbjObmPy3DDZUSJz3NTJiTk="), m54.a("s1:z6/zyURL0GpCdAL3srMZWbn0Hsd6u7atBs7w0O7D1MByEjpEX8LVusNeD3PrvmBAX3TJSVuX5O8=") + K().a);
        }
        this.H0.setValue(Double.valueOf(dF));
        eg3 eg3Var2 = this.z1;
        if (eg3Var2 == null) {
            dValueOf = G();
            dValueOf.getClass();
        } else {
            Double dG = G();
            dG.getClass();
            Double dH = eg3Var2.h(dG.doubleValue() * 1000.0d);
            dValueOf = dH != null ? Double.valueOf(dH.doubleValue() / 1000.0d) : null;
        }
        final Double d2 = dValueOf;
        if (d2 == null) {
            f();
            return;
        }
        final long j = this.I0;
        final long j2 = this.K0 + 1;
        this.K0 = j2;
        this.O0++;
        final int i = this.U0.get();
        final long j3 = K().a;
        this.T0.execute(new Runnable() { // from class: mp3
            @Override // java.lang.Runnable
            public final void run() {
                Player player;
                up3 up3Var = this.g;
                int i2 = i;
                long j4 = j;
                Double d3 = d2;
                long j5 = j2;
                long j6 = j3;
                if (up3Var.U0.get() == i2 && j4 == up3Var.I0 && (player = up3Var.s1) != null) {
                    player.A(d3.doubleValue()).whenComplete((BiConsumer) new rw(27, new io3(up3Var, i2, j4, j5, j6)));
                }
            }
        });
    }

    public final void X() {
        this.R0.post(new uo3(this, 6));
        this.b0 = null;
        Player player = this.s1;
        this.s1 = null;
        this.t1 = 0L;
        if (player != null) {
            try {
                player.n().get();
            } catch (Exception e) {
                this.d1 = e;
                qi qiVar = qi.a;
                qi.e(m54.a("s1:AodeLleWIm9585WmCvU47aiQ87T65WydYYN+lIMLNJwmqNZ94FofFJk7UuI="), m54.a("s1:LKQHpH3TVxJRvdUZEcxUJhvfv0yBNhMMhr+iyG0oF331eEK1wHYG7A=="));
            }
        }
    }

    public final void Y(Player player, vh3 vh3Var, tm1 tm1Var, long j, String str, int i, String str2, boolean z) {
        JSONObject jSONObject;
        gg3 gg3Var;
        String str3;
        Object obj;
        int i2;
        JSONObject jSONObject2 = this.x;
        if (jSONObject2 == null || (jSONObject = this.y) == null || (gg3Var = this.v) == null) {
            return;
        }
        String strV0 = v0(str);
        nh3 nh3Var = (nh3) tm1Var.o;
        long j2 = tm1Var.g;
        if (nh3Var == nh3.o || strV0.length() == 0) {
            return;
        }
        try {
            sh3 sh3Var = (sh3) player.t().get();
            if (sh3Var == null) {
                return;
            }
            ArrayList arrayList = sh3Var.b;
            str3 = "s1:gwLuLUmwmaMxvWwQJvCFsaRW/5R70HpWdo0c/jdHWbkuDxfUqXl4wHhtdGgy4OUdSgvHd6nxxZk=";
            if (this.U0.get() != i) {
                return;
            }
            try {
                if (this.t1 == j && sh3Var.a == j && ((Boolean) this.B.a()).booleanValue()) {
                    m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                    arrayList.getClass();
                    int size = arrayList.size();
                    int i3 = 0;
                    while (true) {
                        if (i3 >= size) {
                            obj = null;
                            break;
                        }
                        obj = arrayList.get(i3);
                        int i4 = i3 + 1;
                        ai3 ai3Var = (ai3) obj;
                        int i5 = size;
                        boolean z2 = ai3Var.l;
                        String str4 = ai3Var.f;
                        if (z2) {
                            i2 = i4;
                            if (!gy1.n(str4, vh3Var == vh3.g ? m54.a("s1:9vE/KjVA3hwcr9gEI5UbEtl78jVONDicVQw24DjmDd7xdjBJvLI=") : m54.a("s1:MI/KfCDPxwtR0UsTnxJV8ADHvsD+y/XljwzKRMtcxtLPXw=="))) {
                                continue;
                            } else {
                                if (gy1.n(str4, m54.a("s1:9vE/KjVA3hwcr9gEI5UbEtl78jVONDicVQw24DjmDd7xdjBJvLI="))) {
                                    break;
                                }
                                Integer num = ai3Var.c;
                                if ((num != null ? num.intValue() : 0) == (vh3Var == vh3.p ? 1 : 0)) {
                                    break;
                                }
                            }
                        } else {
                            i2 = i4;
                        }
                        size = i5;
                        i3 = i2;
                    }
                    m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                    nh3 nh3Var2 = nh3.g;
                    JSONObject jSONObjectB = gi3.B(arrayList, vh3Var, nh3Var == nh3Var2 ? -1L : j2, z);
                    if (jSONObjectB.getBoolean(m54.a("s1:JG00ENWTf6nMmP5/OEFjXo82Q4hMwj9YajI10HbaT9BispSdrZNYITpghIQ="))) {
                        String strR = gi3.R(vh3Var);
                        m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                        tz0 tz0Var = tz0.g;
                        Object objN = fa1.n(m54.a("s1:7u1skT13NtFlsj6uNvSMGZ+DbcPTuiGR4CddEp06wL9Oua0qP7vbDTAgGX59PHOb1aE="), new JSONObject().put(m54.a("s1:ew40n2n3bxNGguvFgPaMhhToWkI3RPcXBsNDffYCWEeAr5ckMtqqtA=="), m54.a("s1:vN1RR/Eblj1qQkmnWfYjpwYYnIFg+O+Sa/530n5XY8mXhtZv0X6KII63MnrK6wKgRXOm4v5WCX4rO6L8")).put(m54.a("s1:2VArilRjwyLHtJn6SZEfQP6jYY4IOMgUoruAm5ElSLBmS2I0"), strR).put(m54.a("s1:K70aJgcChvGrqpuf9yA2pxFcl3mR7wou9H4sErTAzgVQIS5w"), m54.a(nh3Var == nh3Var2 ? "s1:YijQ907msR/0eaJD6HxhroI97jqrRs2MVWqVMy0gyHj5kA==" : "s1:PiyeVCuVuDsBnVmhSiq0tCGpauhD7u5AHQbwAi20rwUTTSmUXvs=")).put(m54.a("s1:jFX7dlAmG/I7yKN9HKXQBBBQnjuAXKKnNCOrIdig2GxJ4R5mgNcIbim8Srg="), String.valueOf(j2)).put(m54.a("s1:wCWlmCtR9Y6e7urINUeKAVOk4W9mwoq8iJwCpOxvM6Ji4hY6dk2VX6tOWXY="), str2).put(m54.a("s1:N258ecO02G8Q/tNhQPbSvppm18CydPUMC5K11qXBhBSOqL+8jbra05T0"), gi3.S(arrayList, tz0Var)));
                        m54.a("s1:1/lFr6r4FbK6DiAQncJeiDPDsJB5+VgpiDbGfXpcgp9De522lWoW6jLE8XV1S+jON7YHcqU12Y45faI0eqvgkJr7vSlVQU4gvhwhuA1UDOwGMAoMruBCTmlpTa14A0Q7pOCNwMCXzn2VxILYtDgFFWEWuYMJnp5HSO37LXCVK384D2tu5yzDcusbDzE=");
                        objN.getClass();
                        JSONObject jSONObject3 = (JSONObject) objN;
                        String strValueOf = nh3Var == nh3Var2 ? "-1" : String.valueOf(j2);
                        m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                        JSONObject jSONObjectD = xf3.d(jSONObject, strV0, strR, strValueOf, gi3.S(arrayList, tz0Var), str2);
                        if (jSONObjectD != null) {
                            if (jSONObject3.optBoolean(m54.a("s1:JG00ENWTf6nMmP5/OEFjXo82Q4hMwj9YajI10HbaT9BispSdrZNYITpghIQ="))) {
                                jSONObjectD.put(m54.a("s1:RZSJzvmb+l2+yMQfJUitGR9UuIPbulslVGsokWdgdUIwAy3+2vIHKSBL"), jSONObject3.get(m54.a("s1:RZSJzvmb+l2+yMQfJUitGR9UuIPbulslVGsokWdgdUIwAy3+2vIHKSBL")));
                            }
                            gg3Var.d(jSONObject2, jSONObject, strV0, strR, jSONObjectD);
                            if (jSONObjectB.getBoolean(m54.a("s1:CPneE9cBEecqfJd9HCVwT/UZ3saO0LvakiW96mOs5N9J43T53jVIfJVSeTWXHOefFDV+EzARI/U="))) {
                                String strA = m54.a("s1:/bcqGsQMlUwQJmpCAectwP6CtNea/85YbqojUdDH0aEOlUnlT6+mBlNEvrnCFA==");
                                m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                gg3Var.d(jSONObject2, jSONObject, strV0, m54.a("s1:/bcqGsQMlUwQJmpCAectwP6CtNea/85YbqojUdDH0aEOlUnlT6+mBlNEvrnCFA=="), xf3.d(jSONObject, strV0, strA, "-1", gi3.S(arrayList, tz0Var), null));
                                return;
                            }
                            return;
                        }
                        if (jSONObject3.optBoolean(m54.a("s1:JG00ENWTf6nMmP5/OEFjXo82Q4hMwj9YajI10HbaT9BispSdrZNYITpghIQ="))) {
                            String string = jSONObject2.getJSONObject(m54.a("s1:NmOEhG0XOOWBSzuoMholvdu97FZ8jWLS5Zaqi0Y1TTjle+k5vSq3I57EkVju5w==")).getString(m54.a("s1:wCWlmCtR9Y6e7urINUeKAVOk4W9mwoq8iJwCpOxvM6Ji4hY6dk2VX6tOWXY="));
                            String strOptString = jSONObject2.optString(m54.a("s1:O+p7ysgRXuVp/mu9vc0pyNC2su413e1rvx5KReq5MLxHwKRXm1aWeWieqDA="));
                            String strOptString2 = jSONObject3.optString(m54.a("s1:RZSJzvmb+l2+yMQfJUitGR9UuIPbulslVGsokWdgdUIwAy3+2vIHKSBL"));
                            if (jSONObject3.isNull(m54.a("s1:RZSJzvmb+l2+yMQfJUitGR9UuIPbulslVGsokWdgdUIwAy3+2vIHKSBL"))) {
                                strOptString2 = null;
                            }
                            gg3Var.b(string, strOptString, strR, strOptString2);
                        }
                        qi qiVar = qi.a;
                        qi.a(m54.a(str3), m54.a("s1:hSJax75t8AttMz3RzofHgdVj5zi4PZlyvZmzCIU4UNMn4aoMIW0hsUBfF5Sw2t4C+SH/2gN7hC9F+LXeQ9CEouCPGjDsDX7eMn7vTxC6EDrcnmgcwF9uKc9Gnr+OZsi96RY=") + ((Object) strR) + m54.a("s1:f1lkjVzg16Lgt9Wd+JDrOn1IV/X3UG5fYb+kIcn5YYeY9Ei+AqyNuw==") + (this.u == null));
                        return;
                    }
                    return;
                }
                return;
            } catch (Exception e) {
                e = e;
            }
        } catch (Exception e2) {
            e = e2;
            str3 = "s1:gwLuLUmwmaMxvWwQJvCFsaRW/5R70HpWdo0c/jdHWbkuDxfUqXl4wHhtdGgy4OUdSgvHd6nxxZk=";
        }
        qi qiVar2 = qi.a;
        y82.t("s1:CrfX5IJLbJN6w0kgsK4f0ARi/GBJh4DuH+A1GW1QX7Arc9Jt8ExZVLnDv01AYgtqURNLT/MxcNEl/NRi1pAUtbY7tmMH6bn8HPhqczBWL8RCyRhEik8=", new StringBuilder(), e.getClass().getSimpleName(), m54.a(str3));
    }

    public final void Z(vh3 vh3Var, String str) throws JSONException {
        String str2;
        if (this.u != null) {
            return;
        }
        int iOrdinal = vh3Var.ordinal();
        if (iOrdinal == 0) {
            str2 = "s1:G8igenSZk44xDQqzHi+SHzrgl8N11SFlwgu5m2Zo4gxcbVV3F2CBk+ODamC4Mq59zcKYiQ==";
        } else if (iOrdinal == 1) {
            str2 = "s1:GULWr3YrEP4zF8BDQ/pfw/E/IQDaD+a50nUHdkuR2xj4uggIBkB0k+FcYSB0hADBZBCfjM89ymRTXpXc/8M=";
        } else {
            if (iOrdinal != 2) {
                vx0.i();
                return;
            }
            str2 = "s1:u99CXtoTgRaaHNfPYvVUHMuvu5PZoJO8vL8zeczWc6MKjazLwS72y883G0fE2yrufmHp/YqqsW3DN1YHnhwP3YMG";
        }
        String strA = m54.a(str2);
        this.S.put(strA, str == null ? JSONObject.NULL : str);
        try {
            this.E.i(strA, str);
        } catch (Exception e) {
            qi qiVar = qi.a;
            y82.t("s1:RztEuASxJSlYtrOiNIDK36VH2sKVGPQj/SZAeuePWBjX0s2vmAnm/uGSy4Kru1pzAAnVQzIf2r9teMDc9HMI6C1nemgPVpA/LUWVkpt0qBw/3dd7c/URU97u", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:gwLuLUmwmaMxvWwQJvCFsaRW/5R70HpWdo0c/jdHWbkuDxfUqXl4wHhtdGgy4OUdSgvHd6nxxZk="));
        }
    }

    public final void a(vh3 vh3Var, JSONObject jSONObject, boolean z) throws JSONException {
        int i;
        pj3 pj3Var;
        ArrayList arrayList;
        long jLongValue;
        List listE;
        String strOptString = jSONObject.optString(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"));
        long jOptLong = jSONObject.optLong("id");
        int iOrdinal = vh3Var.ordinal();
        String strValueOf = "-1";
        if (iOrdinal != 0) {
            if (iOrdinal == 1) {
                this.P = jSONObject;
                if (gy1.n(strOptString, m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8="))) {
                    JSONObject jSONObject2 = this.Q;
                    if (jSONObject2 != null) {
                        jLongValue = jSONObject2.optLong("id");
                    } else {
                        String strOptString2 = this.S.optString(m54.a("s1:u99CXtoTgRaaHNfPYvVUHMuvu5PZoJO8vL8zeczWc6MKjazLwS72y883G0fE2yrufmHp/YqqsW3DN1YHnhwP3YMG"));
                        m54.a("s1:bS90dHkkwXVlWWg2Z9x8x5xit2TOJbreYp7Y3lChPEeDKKR4wbS7tmfK2zc4ViJiLzuochEfCBY=");
                        strOptString2.getClass();
                        Long lB = ct4.B(strOptString2);
                        jLongValue = lB != null ? lB.longValue() : 0L;
                    }
                    pj3 pj3Var2 = this.I;
                    String str = (pj3Var2 == null || (listE = pj3Var2.e()) == null) ? null : (String) listE.get(1);
                    if (str == null) {
                        str = "";
                    }
                    String str2 = str;
                    i = 1;
                    if (gi3.A(jOptLong, jLongValue, "", str2)) {
                        this.Q = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:mO/46UBsIq6g/6wGmcqs8/fxGkgVZUR97LtSOr2egt4irQ=="));
                        if (z) {
                            Z(vh3.p, "-1");
                        }
                    }
                }
            } else {
                if (iOrdinal != 2) {
                    vx0.i();
                    return;
                }
                this.Q = jSONObject;
            }
            i = 1;
        } else {
            i = 1;
            this.R = jSONObject;
        }
        if (z) {
            if (!gy1.n(strOptString, m54.a("s1:mO/46UBsIq6g/6wGmcqs8/fxGkgVZUR97LtSOr2egt4irQ=="))) {
                strValueOf = gy1.n(strOptString, m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")) ? jSONObject.has(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO")) ? (String) this.W.get(jSONObject.getString(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"))) : String.valueOf(jOptLong) : null;
            }
            if (gy1.n(strOptString, m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")) && jSONObject.has(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO")) && strValueOf == null) {
                qi qiVar = qi.a;
                qi.e(m54.a("s1:gwLuLUmwmaMxvWwQJvCFsaRW/5R70HpWdo0c/jdHWbkuDxfUqXl4wHhtdGgy4OUdSgvHd6nxxZk="), m54.a("s1:9lBpqP6Vvhg0nsWSeFq8X9/6088XvnRkADvPrHBg5heXkyRJWlSnmyggY43Gn7/L2EJUnPZ3LEgWO7z9PSmQpSWMIZv99cUrau9j8aLzvEyiKM7T+/kpyoCu"));
            } else {
                Z(vh3Var, strValueOf);
            }
        }
        if (vh3Var == vh3.g || gy1.n(strOptString, m54.a("s1:HsFSqaKTiNJybiRnfYgy3IPDRMq5sQIi80WhrPteKaLWzKGT")) || (pj3Var = this.I) == null) {
            return;
        }
        int i2 = vh3Var == vh3.p ? i : 0;
        if (!gy1.n(strOptString, m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8="))) {
            jOptLong = 0;
        }
        hj3 hj3Var = pj3Var.g[i2];
        if (jOptLong < 0) {
            jOptLong = 0;
        }
        hj3Var.b = jOptLong;
        hj3Var.b("");
        ArrayList arrayListH1 = b60.H1((List) pj3Var.n.getValue());
        arrayListH1.set(i2, Long.valueOf(hj3Var.b));
        pj3Var.n.setValue(arrayListH1);
        ArrayList arrayList2 = pj3Var.m;
        if (arrayList2 != null) {
            arrayList = new ArrayList(arrayList2);
            arrayList.set(i2, Long.valueOf(hj3Var.b));
        } else {
            arrayList = null;
        }
        pj3Var.m = arrayList;
        hj3[] hj3VarArr = pj3Var.g;
        int length = hj3VarArr.length;
        int i3 = 0;
        while (true) {
            if (i3 >= length) {
                pj3Var.m = null;
                break;
            } else if (hj3VarArr[i3].d.length() != 0) {
                break;
            } else {
                i3++;
            }
        }
        hj3[] hj3VarArr2 = pj3Var.g;
        ArrayList arrayList3 = new ArrayList(hj3VarArr2.length);
        for (hj3 hj3Var2 : hj3VarArr2) {
            arrayList3.add(hj3Var2.d);
        }
        pj3Var.q.setValue(arrayList3);
        hj3[] hj3VarArr3 = pj3Var.g;
        ArrayList arrayList4 = new ArrayList(hj3VarArr3.length);
        for (hj3 hj3Var3 : hj3VarArr3) {
            arrayList4.add(Long.valueOf(hj3Var3.b));
        }
        pj3Var.l(arrayList4);
        pj3Var.m(false);
        for (hj3 hj3Var4 : pj3Var.g) {
            if (hj3Var4.c > 0 || hj3Var4.d.length() > 0) {
                pj3Var.i(false);
            }
        }
        if (pj3Var.g[0].b <= 0 || pj3Var.g[0].b != pj3Var.g[i].b) {
            return;
        }
        pj3Var.i(false);
    }

    public final void a0() {
        this.B1 = 0L;
        this.p1 = false;
        this.W0 = false;
        e(false);
        this.q1 = !((Boolean) this.h.a()).booleanValue();
        if (!this.o1 || this.e1 == null) {
            return;
        }
        k0();
    }

    /* JADX WARN: Code duplicated, block: B:39:0x00a8  */
    public final CompletableFuture b(final String str, final String str2, final vh3 vh3Var, final long j, final xu4 xu4Var, boolean z, final String str3, final boolean z2) {
        Object next;
        final long j2;
        boolean z3;
        List listE;
        List listE2;
        ai3 ai3VarK;
        m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO");
        m54.a("s1:cJDIk8tnu2VoRaX19OPJJYsFSsScNUfAk+9kC/k7AJxEYmM2qmY=");
        str2.getClass();
        m54.a("s1:9APGw38pIRmFFr7xugW++AnUv7+waqJhCCY5sZBpeo/SFsH7");
        vh3Var.getClass();
        final CompletableFuture completableFuture = new CompletableFuture();
        final String str4 = this.Z;
        String str5 = null;
        final String str6 = z ? str4 : null;
        if (this.V0 || j != K().a || !K().k() || vh3Var == vh3.g || !this.T.a(xu4Var)) {
            completableFuture.complete(new sp3());
            return completableFuture;
        }
        final int i = this.U0.get();
        ai3 ai3VarO = o(str);
        Long lValueOf = ai3VarO != null ? Long.valueOf(ai3VarO.a) : null;
        pj3 pj3Var = this.I;
        if (pj3Var == null || (ai3VarK = pj3Var.k(0)) == null) {
            Iterator it = N().iterator();
            do {
                if (!it.hasNext()) {
                    next = null;
                    break;
                }
                next = it.next();
                ai3 ai3Var = (ai3) next;
                if (gy1.n(ai3Var.f, m54.a("s1:MI/KfCDPxwtR0UsTnxJV8ADHvsD+y/XljwzKRMtcxtLPXw==")) && ai3Var.l) {
                    Integer num = ai3Var.c;
                    if ((num != null ? num.intValue() : 0) == 0) {
                        z3 = true;
                    } else {
                        z3 = false;
                    }
                } else {
                    z3 = false;
                }
            } while (!z3);
            ai3 ai3Var2 = (ai3) next;
            j2 = ai3Var2 != null ? ai3Var2.a : 0L;
        } else {
            j2 = ai3VarK.a;
        }
        String str7 = (pj3Var == null || (listE2 = pj3Var.e()) == null) ? null : (String) listE2.get(0);
        final String str8 = str7 == null ? "" : str7;
        if (pj3Var != null && (listE = pj3Var.e()) != null) {
            str5 = (String) listE.get(1);
        }
        final String str9 = str5 == null ? "" : str5;
        try {
            final Long l = lValueOf;
            this.T0.execute(new Runnable() { // from class: vo3
                /* JADX WARN: Code duplicated, block: B:108:0x028f  */
                /* JADX WARN: Code duplicated, block: B:113:0x02a2 A[ADDED_TO_REGION] */
                /* JADX WARN: Code duplicated, block: B:145:0x0351  */
                /* JADX WARN: Code duplicated, block: B:148:0x0364 A[Catch: Exception -> 0x0393, TRY_LEAVE, TryCatch #5 {Exception -> 0x0393, blocks: (B:146:0x035e, B:148:0x0364), top: B:184:0x035e }] */
                /* JADX WARN: Code duplicated, block: B:152:0x0374 A[Catch: Exception -> 0x0385, TRY_LEAVE, TryCatch #15 {Exception -> 0x0385, blocks: (B:150:0x036e, B:152:0x0374), top: B:204:0x036e }] */
                /* JADX WARN: Code duplicated, block: B:156:0x0387  */
                /* JADX WARN: Code duplicated, block: B:158:0x038a  */
                /* JADX WARN: Code duplicated, block: B:166:0x0395  */
                /* JADX WARN: Code duplicated, block: B:80:0x021a  */
                /* JADX WARN: Code duplicated, block: B:81:0x021d  */
                /* JADX WARN: Instruction removed from duplicated block: B:81:0x021d, please report this as an issue */
                /* JADX WARN: Multi-variable type inference failed */
                /* JADX WARN: Type inference failed for: r23v10 */
                /* JADX WARN: Type inference failed for: r23v6 */
                /* JADX WARN: Type inference failed for: r23v9 */
                /* JADX WARN: Type inference failed for: r2v48 */
                /* JADX WARN: Type inference failed for: r2v49, types: [boolean] */
                /* JADX WARN: Type inference failed for: r2v50 */
                @Override // java.lang.Runnable
                public final void run() {
                    long j3;
                    i04 i04Var;
                    j04 j04Var;
                    boolean z4;
                    String str10;
                    final long j4;
                    j04 j04Var2;
                    j04 j04Var3;
                    final int i2;
                    boolean z5;
                    boolean z6;
                    String str11;
                    f04 f04Var;
                    boolean z7;
                    j04 j04Var4;
                    j04 j04Var5;
                    j04 j04Var6;
                    boolean z8;
                    ?? r23;
                    up3 up3Var;
                    sh3 sh3Var;
                    Object objE;
                    int i3;
                    boolean z9;
                    Integer num2;
                    sh3 sh3Var2;
                    sh3 sh3Var3;
                    ArrayList arrayList;
                    int i4;
                    Integer num3;
                    long j5 = j2;
                    String str12 = str8;
                    String str13 = str9;
                    String str14 = str4;
                    up3 up3Var2 = this.g;
                    Player player = up3Var2.s1;
                    String str15 = up3Var2.c;
                    d dVar = up3Var2.T;
                    AtomicInteger atomicInteger = up3Var2.U0;
                    int i5 = atomicInteger.get();
                    int i6 = i;
                    final CompletableFuture completableFuture2 = completableFuture;
                    if (i5 == i6 && player != null) {
                        final xu4 xu4Var2 = xu4Var;
                        if (dVar.a(xu4Var2)) {
                            i04 i04Var2 = new i04();
                            Long l2 = l;
                            long jLongValue = l2 != null ? l2.longValue() : 0L;
                            i04Var2.g = jLongValue;
                            final f04 f04Var2 = new f04();
                            j04 j04Var7 = new j04();
                            long j6 = jLongValue;
                            j04Var7.g = str3;
                            j04 j04Var8 = new j04();
                            j04 j04Var9 = new j04();
                            sz0 sz0Var = sz0.g;
                            j04Var9.g = sz0Var;
                            String str16 = str;
                            String str17 = str2;
                            long j7 = j;
                            final vh3 vh3Var2 = vh3Var;
                            up3 up3Var3 = up3Var2;
                            boolean z10 = z2;
                            if (j6 <= 0) {
                                try {
                                    j3 = j7;
                                    try {
                                        Object obj = player.e(m54.a("s1:lkxmtULMMKPWn+kE7EqNyaf5Oo4KuooZ5rSIIgwf0z1YvNjlOUqGICeDAKuS+AtSCFM="), j3, new String[]{str16, str17}, new double[0]).thenApply((Function) new ev2(25)).get();
                                        m54.a("s1:RL7W/MtdNGP866czqj9qUvW1BZgYMrJ0EGwcUs/0bl9ypfPG1dDSzdGH5So=");
                                        obj.getClass();
                                        i04Var2.g = ((Number) obj).longValue();
                                    } catch (Exception e) {
                                        e = e;
                                        i04Var = i04Var2;
                                        j04Var = j04Var7;
                                        z4 = z10;
                                        str10 = str17;
                                        j4 = j3;
                                        j04Var2 = j04Var8;
                                        j04Var3 = j04Var9;
                                        i2 = i6;
                                        qi qiVar = qi.a;
                                        y82.t("s1:6auriwWRmrn0CZ9kkxwjBwSVQPCAfrKN3FM6ckcoDWxmSAgXTmn40eBlz/L5ZbvT9SinBw==", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="));
                                        Handler handler = up3Var3.R0;
                                        final String str18 = str6;
                                        final i04 i04Var3 = i04Var;
                                        final j04 j04Var10 = j04Var;
                                        final String str19 = str16;
                                        final j04 j04Var11 = j04Var2;
                                        final String str20 = str10;
                                        final j04 j04Var12 = j04Var3;
                                        final up3 up3Var4 = up3Var3;
                                        final boolean z11 = z4;
                                        handler.post(new Runnable() { // from class: yo3
                                            @Override // java.lang.Runnable
                                            public final void run() throws JSONException {
                                                Map mapSingletonMap;
                                                up3 up3Var5 = up3Var4;
                                                boolean z12 = up3Var5.V0;
                                                long j8 = j4;
                                                boolean z13 = false;
                                                boolean z14 = !z12 && up3Var5.U0.get() == i2 && up3Var5.K().a == j8;
                                                boolean z15 = z14 && up3Var5.T.a(xu4Var2);
                                                i04 i04Var4 = i04Var3;
                                                f04 f04Var3 = f04Var2;
                                                if (z14 && i04Var4.g > 0) {
                                                    LinkedHashMap linkedHashMap = up3Var5.U;
                                                    String str21 = str19;
                                                    linkedHashMap.put(str21, str20);
                                                    String str22 = str18;
                                                    if (str22 != null) {
                                                        up3Var5.V.put(str21, str22);
                                                    }
                                                    String str23 = (String) j04Var10.g;
                                                    if (str23 != null) {
                                                        up3Var5.W.put(str21, str23);
                                                    }
                                                    Map map = up3Var5.a0;
                                                    Long lValueOf2 = Long.valueOf(i04Var4.g);
                                                    map.getClass();
                                                    if (map.isEmpty()) {
                                                        mapSingletonMap = Collections.singletonMap(lValueOf2, str21);
                                                        mapSingletonMap.getClass();
                                                    } else {
                                                        LinkedHashMap linkedHashMap2 = new LinkedHashMap(map);
                                                        linkedHashMap2.put(lValueOf2, str21);
                                                        mapSingletonMap = linkedHashMap2;
                                                    }
                                                    up3Var5.a0 = mapSingletonMap;
                                                    if (z15 && f04Var3.g) {
                                                        JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")).put("id", i04Var4.g).put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str21);
                                                        m54.a("s1:uZf+OdyW6A/yp2ivdNbkL8GlGcQ0NfqxfMkPw9IFXWpL70RILNRmRaL9gYc=");
                                                        jSONObjectPut.getClass();
                                                        up3Var5.a(vh3Var2, jSONObjectPut, z11);
                                                    }
                                                    if (z15) {
                                                        j04 j04Var13 = j04Var11;
                                                        sh3 sh3Var4 = (sh3) j04Var13.g;
                                                        if (sh3Var4 != null && sh3Var4.a == up3Var5.K().a) {
                                                            Object obj2 = j04Var13.g;
                                                            obj2.getClass();
                                                            ArrayList arrayList2 = ((sh3) obj2).b;
                                                            m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                            arrayList2.getClass();
                                                            up3Var5.f0.setValue(b60.G1(arrayList2));
                                                            up3Var5.o0.setValue((List) j04Var12.g);
                                                        }
                                                    }
                                                    up3Var5.b0();
                                                }
                                                qi qiVar2 = qi.a;
                                                String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                                                boolean z16 = i04Var4.g > 0;
                                                boolean z17 = f04Var3.g && z15;
                                                StringBuilder sb = new StringBuilder();
                                                y82.s("s1:MYwDiD84uurrZSf/89T7hJNNjUM73I97ryP9oY4Zsq82Yx+sQQim2Ma4GZfh60xGfqwEe8kYRyXOGUSAdrfCRJrS/w5bMzzJt/kYfS9VjpAnWSpnRTg=", sb, j8, "s1:F99ebhDddbJBUBFAXCX5cOF01dNn9bJSwAtEgI/6MOocGCCdbKfKvmrRX8dxVZbRMBRxRw==");
                                                sb.append(z16);
                                                sb.append(m54.a("s1:WbnpR8kLfKtfNT/YlYJcbQgdcN0sq1qBaaQOBhJDvj30/uT8/bPObj+rhrQyTmMa"));
                                                sb.append(z17);
                                                sb.append(m54.a("s1:/n/N7UpFZSMSeu0Wlmd1rQaMl/+uEWBUxf0hUW/fTCZb7V9KyWX4z0jWZFZhD88PlAPj5g=="));
                                                sb.append(!z15);
                                                qi.a(strA, sb.toString());
                                                long j9 = i04Var4.g;
                                                if (f04Var3.g && z15) {
                                                    z13 = true;
                                                }
                                                completableFuture2.complete(new sp3(j9, z13));
                                            }
                                        });
                                        up3Var3.v1 = 0;
                                        return;
                                    }
                                } catch (Exception e2) {
                                    e = e2;
                                    j3 = j7;
                                    i04Var = i04Var2;
                                    j04Var = j04Var7;
                                    z4 = z10;
                                    str10 = str17;
                                    j4 = j3;
                                    j04Var2 = j04Var8;
                                    j04Var3 = j04Var9;
                                    i2 = i6;
                                    qi qiVar2 = qi.a;
                                    y82.t("s1:6auriwWRmrn0CZ9kkxwjBwSVQPCAfrKN3FM6ckcoDWxmSAgXTmn40eBlz/L5ZbvT9SinBw==", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="));
                                    Handler handler2 = up3Var3.R0;
                                    final String str110 = str6;
                                    final i04 i04Var4 = i04Var;
                                    final j04 j04Var13 = j04Var;
                                    final String str111 = str16;
                                    final j04 j04Var14 = j04Var2;
                                    final String str21 = str10;
                                    final j04 j04Var15 = j04Var3;
                                    final up3 up3Var5 = up3Var3;
                                    final boolean z12 = z4;
                                    handler2.post(new Runnable() { // from class: yo3
                                        @Override // java.lang.Runnable
                                        public final void run() throws JSONException {
                                            Map mapSingletonMap;
                                            up3 up3Var6 = up3Var5;
                                            boolean z13 = up3Var6.V0;
                                            long j8 = j4;
                                            boolean z14 = false;
                                            boolean z15 = !z13 && up3Var6.U0.get() == i2 && up3Var6.K().a == j8;
                                            boolean z16 = z15 && up3Var6.T.a(xu4Var2);
                                            i04 i04Var5 = i04Var4;
                                            f04 f04Var3 = f04Var2;
                                            if (z15 && i04Var5.g > 0) {
                                                LinkedHashMap linkedHashMap = up3Var6.U;
                                                String str22 = str111;
                                                linkedHashMap.put(str22, str21);
                                                String str23 = str110;
                                                if (str23 != null) {
                                                    up3Var6.V.put(str22, str23);
                                                }
                                                String str24 = (String) j04Var13.g;
                                                if (str24 != null) {
                                                    up3Var6.W.put(str22, str24);
                                                }
                                                Map map = up3Var6.a0;
                                                Long lValueOf2 = Long.valueOf(i04Var5.g);
                                                map.getClass();
                                                if (map.isEmpty()) {
                                                    mapSingletonMap = Collections.singletonMap(lValueOf2, str22);
                                                    mapSingletonMap.getClass();
                                                } else {
                                                    LinkedHashMap linkedHashMap2 = new LinkedHashMap(map);
                                                    linkedHashMap2.put(lValueOf2, str22);
                                                    mapSingletonMap = linkedHashMap2;
                                                }
                                                up3Var6.a0 = mapSingletonMap;
                                                if (z16 && f04Var3.g) {
                                                    JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")).put("id", i04Var5.g).put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str22);
                                                    m54.a("s1:uZf+OdyW6A/yp2ivdNbkL8GlGcQ0NfqxfMkPw9IFXWpL70RILNRmRaL9gYc=");
                                                    jSONObjectPut.getClass();
                                                    up3Var6.a(vh3Var2, jSONObjectPut, z12);
                                                }
                                                if (z16) {
                                                    j04 j04Var16 = j04Var14;
                                                    sh3 sh3Var4 = (sh3) j04Var16.g;
                                                    if (sh3Var4 != null && sh3Var4.a == up3Var6.K().a) {
                                                        Object obj2 = j04Var16.g;
                                                        obj2.getClass();
                                                        ArrayList arrayList2 = ((sh3) obj2).b;
                                                        m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                        arrayList2.getClass();
                                                        up3Var6.f0.setValue(b60.G1(arrayList2));
                                                        up3Var6.o0.setValue((List) j04Var15.g);
                                                    }
                                                }
                                                up3Var6.b0();
                                            }
                                            qi qiVar3 = qi.a;
                                            String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                                            boolean z17 = i04Var5.g > 0;
                                            boolean z18 = f04Var3.g && z16;
                                            StringBuilder sb = new StringBuilder();
                                            y82.s("s1:MYwDiD84uurrZSf/89T7hJNNjUM73I97ryP9oY4Zsq82Yx+sQQim2Ma4GZfh60xGfqwEe8kYRyXOGUSAdrfCRJrS/w5bMzzJt/kYfS9VjpAnWSpnRTg=", sb, j8, "s1:F99ebhDddbJBUBFAXCX5cOF01dNn9bJSwAtEgI/6MOocGCCdbKfKvmrRX8dxVZbRMBRxRw==");
                                            sb.append(z17);
                                            sb.append(m54.a("s1:WbnpR8kLfKtfNT/YlYJcbQgdcN0sq1qBaaQOBhJDvj30/uT8/bPObj+rhrQyTmMa"));
                                            sb.append(z18);
                                            sb.append(m54.a("s1:/n/N7UpFZSMSeu0Wlmd1rQaMl/+uEWBUxf0hUW/fTCZb7V9KyWX4z0jWZFZhD88PlAPj5g=="));
                                            sb.append(!z16);
                                            qi.a(strA, sb.toString());
                                            long j9 = i04Var5.g;
                                            if (f04Var3.g && z16) {
                                                z14 = true;
                                            }
                                            completableFuture2.complete(new sp3(j9, z14));
                                        }
                                    });
                                    up3Var3.v1 = 0;
                                    return;
                                }
                            } else {
                                j3 = j7;
                            }
                            if (vh3Var2 == vh3.o && (sh3Var3 = (sh3) player.t().get()) != null && (arrayList = sh3Var3.b) != null && !arrayList.isEmpty()) {
                                int size = arrayList.size();
                                int i7 = 0;
                                while (true) {
                                    if (i7 >= size) {
                                        z5 = true;
                                        z6 = false;
                                        break;
                                    }
                                    Object obj2 = arrayList.get(i7);
                                    int i8 = i7 + 1;
                                    ArrayList arrayList2 = arrayList;
                                    ai3 ai3Var3 = (ai3) obj2;
                                    if (ai3Var3.l) {
                                        i4 = size;
                                        if (gy1.n(ai3Var3.f, m54.a("s1:MI/KfCDPxwtR0UsTnxJV8ADHvsD+y/XljwzKRMtcxtLPXw==")) && (num3 = ai3Var3.c) != null) {
                                            z5 = true;
                                            if (num3.intValue() == 1 && ai3Var3.a == i04Var2.g) {
                                                z6 = true;
                                                break;
                                            }
                                        }
                                        i7 = i8;
                                        arrayList = arrayList2;
                                        size = i4;
                                    } else {
                                        i4 = size;
                                    }
                                    i7 = i8;
                                    arrayList = arrayList2;
                                    size = i4;
                                }
                            } else {
                                z5 = true;
                                z6 = false;
                                break;
                            }
                            if (vh3Var2 == vh3.p) {
                                try {
                                    str11 = str17;
                                    f04Var = f04Var2;
                                    j04Var4 = j04Var7;
                                    j04Var5 = j04Var8;
                                    j04Var6 = j04Var9;
                                    i04Var = i04Var2;
                                    boolean z13 = z5;
                                    z4 = z10;
                                    try {
                                        z7 = z13;
                                        if (gi3.A(j5, i04Var2.g, str12, str13)) {
                                            z8 = z13 ? 1 : 0;
                                            r23 = z13;
                                        }
                                        if (i04Var.g > 0 && !z8 && dVar.a(xu4Var2)) {
                                            nh3 nh3Var = nh3.p;
                                            long j8 = i04Var.g;
                                            double dOrdinal = vh3Var2.ordinal();
                                            double dOrdinal2 = nh3Var.ordinal();
                                            double[] dArr = new double[3];
                                            dArr[0] = dOrdinal;
                                            dArr[r23] = dOrdinal2;
                                            dArr[2] = j8;
                                            num2 = (Integer) player.G(m54.a("s1:CZN+87AuCwWA1Ti8auQXyRW+Sr6M6iz2dO8O99X80Iq3zIMKMS+WQw=="), j3, new String[0], dArr).get();
                                            if (num2 != null && num2.intValue() == 0 && (sh3Var2 = (sh3) player.t().get()) != null && sh3Var2.a == j3) {
                                                ArrayList arrayList3 = sh3Var2.b;
                                                m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                arrayList3.getClass();
                                                if (gi3.B(arrayList3, vh3Var2, i04Var.g, z6).getBoolean(m54.a("s1:JG00ENWTf6nMmP5/OEFjXo82Q4hMwj9YajI10HbaT9BispSdrZNYITpghIQ="))) {
                                                }
                                            }
                                            f04Var.g = ;
                                        }
                                        if (f04Var.g || !z4) {
                                            str10 = str11;
                                            up3Var = up3Var3;
                                            j04Var = j04Var4;
                                            if (f04Var.g || !z4) {
                                                i04Var = i04Var;
                                                f04Var2 = f04Var;
                                                vh3Var2 = vh3Var2;
                                                xu4Var2 = xu4Var2;
                                                j4 = j3;
                                                up3Var3 = up3Var;
                                                i2 = i6;
                                                str16 = str16;
                                                try {
                                                    if (i04Var.g > 0) {
                                                        Object obj3 = player.t().get();
                                                        j04Var2 = j04Var5;
                                                        try {
                                                            j04Var2.g = obj3;
                                                            sh3Var = (sh3) obj3;
                                                            if (sh3Var != null) {
                                                                DateTimeFormatter dateTimeFormatter = vp2.a;
                                                                objE = vp2.e(sh3Var, (aq2) up3Var3.r0.getValue(), up3Var3.m0);
                                                            } else {
                                                                objE = null;
                                                            }
                                                            if (objE == null) {
                                                                objE = sz0Var;
                                                            }
                                                            j04Var3 = j04Var6;
                                                            try {
                                                                j04Var3.g = objE;
                                                            } catch (Exception e3) {
                                                                e = e3;
                                                                qi qiVar3 = qi.a;
                                                                y82.t("s1:6auriwWRmrn0CZ9kkxwjBwSVQPCAfrKN3FM6ckcoDWxmSAgXTmn40eBlz/L5ZbvT9SinBw==", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="));
                                                            }
                                                        } catch (Exception e4) {
                                                            e = e4;
                                                            j04Var3 = j04Var6;
                                                            qi qiVar4 = qi.a;
                                                            y82.t("s1:6auriwWRmrn0CZ9kkxwjBwSVQPCAfrKN3FM6ckcoDWxmSAgXTmn40eBlz/L5ZbvT9SinBw==", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="));
                                                            Handler handler3 = up3Var3.R0;
                                                            final String str112 = str6;
                                                            final i04 i04Var5 = i04Var;
                                                            final j04 j04Var16 = j04Var;
                                                            final String str113 = str16;
                                                            final j04 j04Var17 = j04Var2;
                                                            final String str22 = str10;
                                                            final j04 j04Var18 = j04Var3;
                                                            final up3 up3Var6 = up3Var3;
                                                            final boolean z14 = z4;
                                                            handler3.post(new Runnable() { // from class: yo3
                                                                @Override // java.lang.Runnable
                                                                public final void run() throws JSONException {
                                                                    Map mapSingletonMap;
                                                                    up3 up3Var7 = up3Var6;
                                                                    boolean z15 = up3Var7.V0;
                                                                    long j9 = j4;
                                                                    boolean z16 = false;
                                                                    boolean z17 = !z15 && up3Var7.U0.get() == i2 && up3Var7.K().a == j9;
                                                                    boolean z18 = z17 && up3Var7.T.a(xu4Var2);
                                                                    i04 i04Var6 = i04Var5;
                                                                    f04 f04Var3 = f04Var2;
                                                                    if (z17 && i04Var6.g > 0) {
                                                                        LinkedHashMap linkedHashMap = up3Var7.U;
                                                                        String str23 = str113;
                                                                        linkedHashMap.put(str23, str22);
                                                                        String str24 = str112;
                                                                        if (str24 != null) {
                                                                            up3Var7.V.put(str23, str24);
                                                                        }
                                                                        String str25 = (String) j04Var16.g;
                                                                        if (str25 != null) {
                                                                            up3Var7.W.put(str23, str25);
                                                                        }
                                                                        Map map = up3Var7.a0;
                                                                        Long lValueOf2 = Long.valueOf(i04Var6.g);
                                                                        map.getClass();
                                                                        if (map.isEmpty()) {
                                                                            mapSingletonMap = Collections.singletonMap(lValueOf2, str23);
                                                                            mapSingletonMap.getClass();
                                                                        } else {
                                                                            LinkedHashMap linkedHashMap2 = new LinkedHashMap(map);
                                                                            linkedHashMap2.put(lValueOf2, str23);
                                                                            mapSingletonMap = linkedHashMap2;
                                                                        }
                                                                        up3Var7.a0 = mapSingletonMap;
                                                                        if (z18 && f04Var3.g) {
                                                                            JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")).put("id", i04Var6.g).put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str23);
                                                                            m54.a("s1:uZf+OdyW6A/yp2ivdNbkL8GlGcQ0NfqxfMkPw9IFXWpL70RILNRmRaL9gYc=");
                                                                            jSONObjectPut.getClass();
                                                                            up3Var7.a(vh3Var2, jSONObjectPut, z14);
                                                                        }
                                                                        if (z18) {
                                                                            j04 j04Var19 = j04Var17;
                                                                            sh3 sh3Var4 = (sh3) j04Var19.g;
                                                                            if (sh3Var4 != null && sh3Var4.a == up3Var7.K().a) {
                                                                                Object obj4 = j04Var19.g;
                                                                                obj4.getClass();
                                                                                ArrayList arrayList4 = ((sh3) obj4).b;
                                                                                m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                                                arrayList4.getClass();
                                                                                up3Var7.f0.setValue(b60.G1(arrayList4));
                                                                                up3Var7.o0.setValue((List) j04Var18.g);
                                                                            }
                                                                        }
                                                                        up3Var7.b0();
                                                                    }
                                                                    qi qiVar5 = qi.a;
                                                                    String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                                                                    boolean z19 = i04Var6.g > 0;
                                                                    boolean z110 = f04Var3.g && z18;
                                                                    StringBuilder sb = new StringBuilder();
                                                                    y82.s("s1:MYwDiD84uurrZSf/89T7hJNNjUM73I97ryP9oY4Zsq82Yx+sQQim2Ma4GZfh60xGfqwEe8kYRyXOGUSAdrfCRJrS/w5bMzzJt/kYfS9VjpAnWSpnRTg=", sb, j9, "s1:F99ebhDddbJBUBFAXCX5cOF01dNn9bJSwAtEgI/6MOocGCCdbKfKvmrRX8dxVZbRMBRxRw==");
                                                                    sb.append(z19);
                                                                    sb.append(m54.a("s1:WbnpR8kLfKtfNT/YlYJcbQgdcN0sq1qBaaQOBhJDvj30/uT8/bPObj+rhrQyTmMa"));
                                                                    sb.append(z110);
                                                                    sb.append(m54.a("s1:/n/N7UpFZSMSeu0Wlmd1rQaMl/+uEWBUxf0hUW/fTCZb7V9KyWX4z0jWZFZhD88PlAPj5g=="));
                                                                    sb.append(!z18);
                                                                    qi.a(strA, sb.toString());
                                                                    long j10 = i04Var6.g;
                                                                    if (f04Var3.g && z18) {
                                                                        z16 = true;
                                                                    }
                                                                    completableFuture2.complete(new sp3(j10, z16));
                                                                }
                                                            });
                                                            up3Var3.v1 = 0;
                                                            return;
                                                        }
                                                    } else {
                                                        j04Var2 = j04Var5;
                                                        j04Var3 = j04Var6;
                                                    }
                                                } catch (Exception e5) {
                                                    e = e5;
                                                    j04Var2 = j04Var5;
                                                    j04Var3 = j04Var6;
                                                    qi qiVar5 = qi.a;
                                                    y82.t("s1:6auriwWRmrn0CZ9kkxwjBwSVQPCAfrKN3FM6ckcoDWxmSAgXTmn40eBlz/L5ZbvT9SinBw==", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="));
                                                    Handler handler4 = up3Var3.R0;
                                                    final String str114 = str6;
                                                    final i04 i04Var6 = i04Var;
                                                    final j04 j04Var19 = j04Var;
                                                    final String str115 = str16;
                                                    final j04 j04Var110 = j04Var2;
                                                    final String str23 = str10;
                                                    final j04 j04Var111 = j04Var3;
                                                    final up3 up3Var7 = up3Var3;
                                                    final boolean z15 = z4;
                                                    handler4.post(new Runnable() { // from class: yo3
                                                        @Override // java.lang.Runnable
                                                        public final void run() throws JSONException {
                                                            Map mapSingletonMap;
                                                            up3 up3Var8 = up3Var7;
                                                            boolean z16 = up3Var8.V0;
                                                            long j9 = j4;
                                                            boolean z17 = false;
                                                            boolean z18 = !z16 && up3Var8.U0.get() == i2 && up3Var8.K().a == j9;
                                                            boolean z19 = z18 && up3Var8.T.a(xu4Var2);
                                                            i04 i04Var7 = i04Var6;
                                                            f04 f04Var3 = f04Var2;
                                                            if (z18 && i04Var7.g > 0) {
                                                                LinkedHashMap linkedHashMap = up3Var8.U;
                                                                String str24 = str115;
                                                                linkedHashMap.put(str24, str23);
                                                                String str25 = str114;
                                                                if (str25 != null) {
                                                                    up3Var8.V.put(str24, str25);
                                                                }
                                                                String str26 = (String) j04Var19.g;
                                                                if (str26 != null) {
                                                                    up3Var8.W.put(str24, str26);
                                                                }
                                                                Map map = up3Var8.a0;
                                                                Long lValueOf2 = Long.valueOf(i04Var7.g);
                                                                map.getClass();
                                                                if (map.isEmpty()) {
                                                                    mapSingletonMap = Collections.singletonMap(lValueOf2, str24);
                                                                    mapSingletonMap.getClass();
                                                                } else {
                                                                    LinkedHashMap linkedHashMap2 = new LinkedHashMap(map);
                                                                    linkedHashMap2.put(lValueOf2, str24);
                                                                    mapSingletonMap = linkedHashMap2;
                                                                }
                                                                up3Var8.a0 = mapSingletonMap;
                                                                if (z19 && f04Var3.g) {
                                                                    JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")).put("id", i04Var7.g).put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str24);
                                                                    m54.a("s1:uZf+OdyW6A/yp2ivdNbkL8GlGcQ0NfqxfMkPw9IFXWpL70RILNRmRaL9gYc=");
                                                                    jSONObjectPut.getClass();
                                                                    up3Var8.a(vh3Var2, jSONObjectPut, z15);
                                                                }
                                                                if (z19) {
                                                                    j04 j04Var112 = j04Var110;
                                                                    sh3 sh3Var4 = (sh3) j04Var112.g;
                                                                    if (sh3Var4 != null && sh3Var4.a == up3Var8.K().a) {
                                                                        Object obj4 = j04Var112.g;
                                                                        obj4.getClass();
                                                                        ArrayList arrayList4 = ((sh3) obj4).b;
                                                                        m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                                        arrayList4.getClass();
                                                                        up3Var8.f0.setValue(b60.G1(arrayList4));
                                                                        up3Var8.o0.setValue((List) j04Var111.g);
                                                                    }
                                                                }
                                                                up3Var8.b0();
                                                            }
                                                            qi qiVar6 = qi.a;
                                                            String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                                                            boolean z110 = i04Var7.g > 0;
                                                            boolean z111 = f04Var3.g && z19;
                                                            StringBuilder sb = new StringBuilder();
                                                            y82.s("s1:MYwDiD84uurrZSf/89T7hJNNjUM73I97ryP9oY4Zsq82Yx+sQQim2Ma4GZfh60xGfqwEe8kYRyXOGUSAdrfCRJrS/w5bMzzJt/kYfS9VjpAnWSpnRTg=", sb, j9, "s1:F99ebhDddbJBUBFAXCX5cOF01dNn9bJSwAtEgI/6MOocGCCdbKfKvmrRX8dxVZbRMBRxRw==");
                                                            sb.append(z110);
                                                            sb.append(m54.a("s1:WbnpR8kLfKtfNT/YlYJcbQgdcN0sq1qBaaQOBhJDvj30/uT8/bPObj+rhrQyTmMa"));
                                                            sb.append(z111);
                                                            sb.append(m54.a("s1:/n/N7UpFZSMSeu0Wlmd1rQaMl/+uEWBUxf0hUW/fTCZb7V9KyWX4z0jWZFZhD88PlAPj5g=="));
                                                            sb.append(!z19);
                                                            qi.a(strA, sb.toString());
                                                            long j10 = i04Var7.g;
                                                            if (f04Var3.g && z19) {
                                                                z17 = true;
                                                            }
                                                            completableFuture2.complete(new sp3(j10, z17));
                                                        }
                                                    });
                                                    up3Var3.v1 = 0;
                                                    return;
                                                }
                                            } else {
                                                try {
                                                    if (dVar.a(xu4Var2)) {
                                                        sh3 sh3Var4 = (sh3) player.t().get();
                                                        if (sh3Var4 != null) {
                                                            try {
                                                                ArrayList arrayList4 = sh3Var4.b;
                                                                m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                                arrayList4.getClass();
                                                                JSONArray jSONArrayS = gi3.S(arrayList4, tz0.g);
                                                                f04Var2 = f04Var;
                                                                try {
                                                                    String strR = gi3.R(vh3Var2);
                                                                    String strValueOf = String.valueOf(i04Var.g);
                                                                    i3 = i6;
                                                                    vh3Var2 = vh3Var2;
                                                                    xu4Var2 = xu4Var2;
                                                                    z9 = z6;
                                                                    i04Var = i04Var;
                                                                    up3Var3 = up3Var;
                                                                    try {
                                                                        up3Var3.P(jSONArrayS, strR, strValueOf, (String) j04Var.g, z9);
                                                                    } catch (Exception e6) {
                                                                        e = e6;
                                                                        str16 = str16;
                                                                        j4 = j3;
                                                                        j04Var2 = j04Var5;
                                                                        j04Var3 = j04Var6;
                                                                        i2 = i3;
                                                                        qi qiVar6 = qi.a;
                                                                        y82.t("s1:6auriwWRmrn0CZ9kkxwjBwSVQPCAfrKN3FM6ckcoDWxmSAgXTmn40eBlz/L5ZbvT9SinBw==", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="));
                                                                        Handler handler5 = up3Var3.R0;
                                                                        final String str116 = str6;
                                                                        final i04 i04Var7 = i04Var;
                                                                        final j04 j04Var112 = j04Var;
                                                                        final String str117 = str16;
                                                                        final j04 j04Var113 = j04Var2;
                                                                        final String str24 = str10;
                                                                        final j04 j04Var114 = j04Var3;
                                                                        final up3 up3Var8 = up3Var3;
                                                                        final boolean z16 = z4;
                                                                        handler5.post(new Runnable() { // from class: yo3
                                                                            @Override // java.lang.Runnable
                                                                            public final void run() throws JSONException {
                                                                                Map mapSingletonMap;
                                                                                up3 up3Var9 = up3Var8;
                                                                                boolean z17 = up3Var9.V0;
                                                                                long j9 = j4;
                                                                                boolean z18 = false;
                                                                                boolean z19 = !z17 && up3Var9.U0.get() == i2 && up3Var9.K().a == j9;
                                                                                boolean z110 = z19 && up3Var9.T.a(xu4Var2);
                                                                                i04 i04Var8 = i04Var7;
                                                                                f04 f04Var3 = f04Var2;
                                                                                if (z19 && i04Var8.g > 0) {
                                                                                    LinkedHashMap linkedHashMap = up3Var9.U;
                                                                                    String str25 = str117;
                                                                                    linkedHashMap.put(str25, str24);
                                                                                    String str26 = str116;
                                                                                    if (str26 != null) {
                                                                                        up3Var9.V.put(str25, str26);
                                                                                    }
                                                                                    String str27 = (String) j04Var112.g;
                                                                                    if (str27 != null) {
                                                                                        up3Var9.W.put(str25, str27);
                                                                                    }
                                                                                    Map map = up3Var9.a0;
                                                                                    Long lValueOf2 = Long.valueOf(i04Var8.g);
                                                                                    map.getClass();
                                                                                    if (map.isEmpty()) {
                                                                                        mapSingletonMap = Collections.singletonMap(lValueOf2, str25);
                                                                                        mapSingletonMap.getClass();
                                                                                    } else {
                                                                                        LinkedHashMap linkedHashMap2 = new LinkedHashMap(map);
                                                                                        linkedHashMap2.put(lValueOf2, str25);
                                                                                        mapSingletonMap = linkedHashMap2;
                                                                                    }
                                                                                    up3Var9.a0 = mapSingletonMap;
                                                                                    if (z110 && f04Var3.g) {
                                                                                        JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")).put("id", i04Var8.g).put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str25);
                                                                                        m54.a("s1:uZf+OdyW6A/yp2ivdNbkL8GlGcQ0NfqxfMkPw9IFXWpL70RILNRmRaL9gYc=");
                                                                                        jSONObjectPut.getClass();
                                                                                        up3Var9.a(vh3Var2, jSONObjectPut, z16);
                                                                                    }
                                                                                    if (z110) {
                                                                                        j04 j04Var115 = j04Var113;
                                                                                        sh3 sh3Var5 = (sh3) j04Var115.g;
                                                                                        if (sh3Var5 != null && sh3Var5.a == up3Var9.K().a) {
                                                                                            Object obj4 = j04Var115.g;
                                                                                            obj4.getClass();
                                                                                            ArrayList arrayList5 = ((sh3) obj4).b;
                                                                                            m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                                                            arrayList5.getClass();
                                                                                            up3Var9.f0.setValue(b60.G1(arrayList5));
                                                                                            up3Var9.o0.setValue((List) j04Var114.g);
                                                                                        }
                                                                                    }
                                                                                    up3Var9.b0();
                                                                                }
                                                                                qi qiVar7 = qi.a;
                                                                                String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                                                                                boolean z111 = i04Var8.g > 0;
                                                                                boolean z112 = f04Var3.g && z110;
                                                                                StringBuilder sb = new StringBuilder();
                                                                                y82.s("s1:MYwDiD84uurrZSf/89T7hJNNjUM73I97ryP9oY4Zsq82Yx+sQQim2Ma4GZfh60xGfqwEe8kYRyXOGUSAdrfCRJrS/w5bMzzJt/kYfS9VjpAnWSpnRTg=", sb, j9, "s1:F99ebhDddbJBUBFAXCX5cOF01dNn9bJSwAtEgI/6MOocGCCdbKfKvmrRX8dxVZbRMBRxRw==");
                                                                                sb.append(z111);
                                                                                sb.append(m54.a("s1:WbnpR8kLfKtfNT/YlYJcbQgdcN0sq1qBaaQOBhJDvj30/uT8/bPObj+rhrQyTmMa"));
                                                                                sb.append(z112);
                                                                                sb.append(m54.a("s1:/n/N7UpFZSMSeu0Wlmd1rQaMl/+uEWBUxf0hUW/fTCZb7V9KyWX4z0jWZFZhD88PlAPj5g=="));
                                                                                sb.append(!z110);
                                                                                qi.a(strA, sb.toString());
                                                                                long j10 = i04Var8.g;
                                                                                if (f04Var3.g && z110) {
                                                                                    z18 = true;
                                                                                }
                                                                                completableFuture2.complete(new sp3(j10, z18));
                                                                            }
                                                                        });
                                                                        up3Var3.v1 = 0;
                                                                        return;
                                                                    }
                                                                } catch (Exception e7) {
                                                                    e = e7;
                                                                    i3 = i6;
                                                                    vh3Var2 = vh3Var2;
                                                                    xu4Var2 = xu4Var2;
                                                                    up3Var3 = up3Var;
                                                                    str16 = str16;
                                                                    j4 = j3;
                                                                    j04Var2 = j04Var5;
                                                                    j04Var3 = j04Var6;
                                                                    i2 = i3;
                                                                    qi qiVar7 = qi.a;
                                                                    y82.t("s1:6auriwWRmrn0CZ9kkxwjBwSVQPCAfrKN3FM6ckcoDWxmSAgXTmn40eBlz/L5ZbvT9SinBw==", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="));
                                                                    Handler handler6 = up3Var3.R0;
                                                                    final String str118 = str6;
                                                                    final i04 i04Var8 = i04Var;
                                                                    final j04 j04Var115 = j04Var;
                                                                    final String str119 = str16;
                                                                    final j04 j04Var116 = j04Var2;
                                                                    final String str25 = str10;
                                                                    final j04 j04Var117 = j04Var3;
                                                                    final up3 up3Var9 = up3Var3;
                                                                    final boolean z17 = z4;
                                                                    handler6.post(new Runnable() { // from class: yo3
                                                                        @Override // java.lang.Runnable
                                                                        public final void run() throws JSONException {
                                                                            Map mapSingletonMap;
                                                                            up3 up3Var10 = up3Var9;
                                                                            boolean z18 = up3Var10.V0;
                                                                            long j9 = j4;
                                                                            boolean z19 = false;
                                                                            boolean z110 = !z18 && up3Var10.U0.get() == i2 && up3Var10.K().a == j9;
                                                                            boolean z111 = z110 && up3Var10.T.a(xu4Var2);
                                                                            i04 i04Var9 = i04Var8;
                                                                            f04 f04Var3 = f04Var2;
                                                                            if (z110 && i04Var9.g > 0) {
                                                                                LinkedHashMap linkedHashMap = up3Var10.U;
                                                                                String str26 = str119;
                                                                                linkedHashMap.put(str26, str25);
                                                                                String str27 = str118;
                                                                                if (str27 != null) {
                                                                                    up3Var10.V.put(str26, str27);
                                                                                }
                                                                                String str28 = (String) j04Var115.g;
                                                                                if (str28 != null) {
                                                                                    up3Var10.W.put(str26, str28);
                                                                                }
                                                                                Map map = up3Var10.a0;
                                                                                Long lValueOf2 = Long.valueOf(i04Var9.g);
                                                                                map.getClass();
                                                                                if (map.isEmpty()) {
                                                                                    mapSingletonMap = Collections.singletonMap(lValueOf2, str26);
                                                                                    mapSingletonMap.getClass();
                                                                                } else {
                                                                                    LinkedHashMap linkedHashMap2 = new LinkedHashMap(map);
                                                                                    linkedHashMap2.put(lValueOf2, str26);
                                                                                    mapSingletonMap = linkedHashMap2;
                                                                                }
                                                                                up3Var10.a0 = mapSingletonMap;
                                                                                if (z111 && f04Var3.g) {
                                                                                    JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")).put("id", i04Var9.g).put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str26);
                                                                                    m54.a("s1:uZf+OdyW6A/yp2ivdNbkL8GlGcQ0NfqxfMkPw9IFXWpL70RILNRmRaL9gYc=");
                                                                                    jSONObjectPut.getClass();
                                                                                    up3Var10.a(vh3Var2, jSONObjectPut, z17);
                                                                                }
                                                                                if (z111) {
                                                                                    j04 j04Var118 = j04Var116;
                                                                                    sh3 sh3Var5 = (sh3) j04Var118.g;
                                                                                    if (sh3Var5 != null && sh3Var5.a == up3Var10.K().a) {
                                                                                        Object obj4 = j04Var118.g;
                                                                                        obj4.getClass();
                                                                                        ArrayList arrayList5 = ((sh3) obj4).b;
                                                                                        m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                                                        arrayList5.getClass();
                                                                                        up3Var10.f0.setValue(b60.G1(arrayList5));
                                                                                        up3Var10.o0.setValue((List) j04Var117.g);
                                                                                    }
                                                                                }
                                                                                up3Var10.b0();
                                                                            }
                                                                            qi qiVar8 = qi.a;
                                                                            String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                                                                            boolean z112 = i04Var9.g > 0;
                                                                            boolean z113 = f04Var3.g && z111;
                                                                            StringBuilder sb = new StringBuilder();
                                                                            y82.s("s1:MYwDiD84uurrZSf/89T7hJNNjUM73I97ryP9oY4Zsq82Yx+sQQim2Ma4GZfh60xGfqwEe8kYRyXOGUSAdrfCRJrS/w5bMzzJt/kYfS9VjpAnWSpnRTg=", sb, j9, "s1:F99ebhDddbJBUBFAXCX5cOF01dNn9bJSwAtEgI/6MOocGCCdbKfKvmrRX8dxVZbRMBRxRw==");
                                                                            sb.append(z112);
                                                                            sb.append(m54.a("s1:WbnpR8kLfKtfNT/YlYJcbQgdcN0sq1qBaaQOBhJDvj30/uT8/bPObj+rhrQyTmMa"));
                                                                            sb.append(z113);
                                                                            sb.append(m54.a("s1:/n/N7UpFZSMSeu0Wlmd1rQaMl/+uEWBUxf0hUW/fTCZb7V9KyWX4z0jWZFZhD88PlAPj5g=="));
                                                                            sb.append(!z111);
                                                                            qi.a(strA, sb.toString());
                                                                            long j10 = i04Var9.g;
                                                                            if (f04Var3.g && z111) {
                                                                                z19 = true;
                                                                            }
                                                                            completableFuture2.complete(new sp3(j10, z19));
                                                                        }
                                                                    });
                                                                    up3Var3.v1 = 0;
                                                                    return;
                                                                }
                                                            } catch (Exception e8) {
                                                                e = e8;
                                                                f04Var2 = f04Var;
                                                            }
                                                        } else {
                                                            f04Var2 = f04Var;
                                                            vh3Var2 = vh3Var2;
                                                            xu4Var2 = xu4Var2;
                                                            z9 = z6;
                                                            i04Var = i04Var;
                                                            i3 = i6;
                                                            up3Var3 = up3Var;
                                                        }
                                                        try {
                                                            long j9 = j3;
                                                            str16 = str16;
                                                            vh3 vh3Var3 = vh3Var2;
                                                            try {
                                                                up3Var3.Y(player, vh3Var3, new tm1(i04Var.g, nh3.p), j9, str14, i3, (String) j04Var.g, z9);
                                                                vh3Var2 = vh3Var3;
                                                                j4 = j9;
                                                                i2 = i3;
                                                                player = player;
                                                            } catch (Exception e9) {
                                                                e = e9;
                                                                vh3Var2 = vh3Var3;
                                                                j4 = j9;
                                                                i2 = i3;
                                                                j04Var2 = j04Var5;
                                                                j04Var3 = j04Var6;
                                                                qi qiVar8 = qi.a;
                                                                y82.t("s1:6auriwWRmrn0CZ9kkxwjBwSVQPCAfrKN3FM6ckcoDWxmSAgXTmn40eBlz/L5ZbvT9SinBw==", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="));
                                                                Handler handler7 = up3Var3.R0;
                                                                final String str1110 = str6;
                                                                final i04 i04Var9 = i04Var;
                                                                final j04 j04Var118 = j04Var;
                                                                final String str1111 = str16;
                                                                final j04 j04Var119 = j04Var2;
                                                                final String str26 = str10;
                                                                final j04 j04Var1110 = j04Var3;
                                                                final up3 up3Var10 = up3Var3;
                                                                final boolean z18 = z4;
                                                                handler7.post(new Runnable() { // from class: yo3
                                                                    @Override // java.lang.Runnable
                                                                    public final void run() throws JSONException {
                                                                        Map mapSingletonMap;
                                                                        up3 up3Var11 = up3Var10;
                                                                        boolean z19 = up3Var11.V0;
                                                                        long j10 = j4;
                                                                        boolean z110 = false;
                                                                        boolean z111 = !z19 && up3Var11.U0.get() == i2 && up3Var11.K().a == j10;
                                                                        boolean z112 = z111 && up3Var11.T.a(xu4Var2);
                                                                        i04 i04Var10 = i04Var9;
                                                                        f04 f04Var3 = f04Var2;
                                                                        if (z111 && i04Var10.g > 0) {
                                                                            LinkedHashMap linkedHashMap = up3Var11.U;
                                                                            String str27 = str1111;
                                                                            linkedHashMap.put(str27, str26);
                                                                            String str28 = str1110;
                                                                            if (str28 != null) {
                                                                                up3Var11.V.put(str27, str28);
                                                                            }
                                                                            String str29 = (String) j04Var118.g;
                                                                            if (str29 != null) {
                                                                                up3Var11.W.put(str27, str29);
                                                                            }
                                                                            Map map = up3Var11.a0;
                                                                            Long lValueOf2 = Long.valueOf(i04Var10.g);
                                                                            map.getClass();
                                                                            if (map.isEmpty()) {
                                                                                mapSingletonMap = Collections.singletonMap(lValueOf2, str27);
                                                                                mapSingletonMap.getClass();
                                                                            } else {
                                                                                LinkedHashMap linkedHashMap2 = new LinkedHashMap(map);
                                                                                linkedHashMap2.put(lValueOf2, str27);
                                                                                mapSingletonMap = linkedHashMap2;
                                                                            }
                                                                            up3Var11.a0 = mapSingletonMap;
                                                                            if (z112 && f04Var3.g) {
                                                                                JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")).put("id", i04Var10.g).put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str27);
                                                                                m54.a("s1:uZf+OdyW6A/yp2ivdNbkL8GlGcQ0NfqxfMkPw9IFXWpL70RILNRmRaL9gYc=");
                                                                                jSONObjectPut.getClass();
                                                                                up3Var11.a(vh3Var2, jSONObjectPut, z18);
                                                                            }
                                                                            if (z112) {
                                                                                j04 j04Var1111 = j04Var119;
                                                                                sh3 sh3Var5 = (sh3) j04Var1111.g;
                                                                                if (sh3Var5 != null && sh3Var5.a == up3Var11.K().a) {
                                                                                    Object obj4 = j04Var1111.g;
                                                                                    obj4.getClass();
                                                                                    ArrayList arrayList5 = ((sh3) obj4).b;
                                                                                    m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                                                    arrayList5.getClass();
                                                                                    up3Var11.f0.setValue(b60.G1(arrayList5));
                                                                                    up3Var11.o0.setValue((List) j04Var1110.g);
                                                                                }
                                                                            }
                                                                            up3Var11.b0();
                                                                        }
                                                                        qi qiVar9 = qi.a;
                                                                        String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                                                                        boolean z113 = i04Var10.g > 0;
                                                                        boolean z114 = f04Var3.g && z112;
                                                                        StringBuilder sb = new StringBuilder();
                                                                        y82.s("s1:MYwDiD84uurrZSf/89T7hJNNjUM73I97ryP9oY4Zsq82Yx+sQQim2Ma4GZfh60xGfqwEe8kYRyXOGUSAdrfCRJrS/w5bMzzJt/kYfS9VjpAnWSpnRTg=", sb, j10, "s1:F99ebhDddbJBUBFAXCX5cOF01dNn9bJSwAtEgI/6MOocGCCdbKfKvmrRX8dxVZbRMBRxRw==");
                                                                        sb.append(z113);
                                                                        sb.append(m54.a("s1:WbnpR8kLfKtfNT/YlYJcbQgdcN0sq1qBaaQOBhJDvj30/uT8/bPObj+rhrQyTmMa"));
                                                                        sb.append(z114);
                                                                        sb.append(m54.a("s1:/n/N7UpFZSMSeu0Wlmd1rQaMl/+uEWBUxf0hUW/fTCZb7V9KyWX4z0jWZFZhD88PlAPj5g=="));
                                                                        sb.append(!z112);
                                                                        qi.a(strA, sb.toString());
                                                                        long j11 = i04Var10.g;
                                                                        if (f04Var3.g && z112) {
                                                                            z110 = true;
                                                                        }
                                                                        completableFuture2.complete(new sp3(j11, z110));
                                                                    }
                                                                });
                                                                up3Var3.v1 = 0;
                                                                return;
                                                            }
                                                        } catch (Exception e10) {
                                                            e = e10;
                                                            str16 = str16;
                                                            j4 = j3;
                                                        }
                                                    } else {
                                                        i04Var = i04Var;
                                                        f04Var2 = f04Var;
                                                        vh3Var2 = vh3Var2;
                                                        xu4Var2 = xu4Var2;
                                                        j4 = j3;
                                                        up3Var3 = up3Var;
                                                        i2 = i6;
                                                        str16 = str16;
                                                    }
                                                    if (i04Var.g > 0) {
                                                        Object obj4 = player.t().get();
                                                        j04Var2 = j04Var5;
                                                        j04Var2.g = obj4;
                                                        sh3Var = (sh3) obj4;
                                                        if (sh3Var != null) {
                                                            DateTimeFormatter dateTimeFormatter2 = vp2.a;
                                                            objE = vp2.e(sh3Var, (aq2) up3Var3.r0.getValue(), up3Var3.m0);
                                                        } else {
                                                            objE = null;
                                                        }
                                                        if (objE == null) {
                                                            objE = sz0Var;
                                                        }
                                                        j04Var3 = j04Var6;
                                                        j04Var3.g = objE;
                                                    } else {
                                                        j04Var2 = j04Var5;
                                                        j04Var3 = j04Var6;
                                                    }
                                                } catch (Exception e11) {
                                                    e = e11;
                                                    i04Var = i04Var;
                                                    f04Var2 = f04Var;
                                                    vh3Var2 = vh3Var2;
                                                    xu4Var2 = xu4Var2;
                                                    j4 = j3;
                                                    up3Var3 = up3Var;
                                                    i2 = i6;
                                                    str16 = str16;
                                                }
                                            }
                                        } else {
                                            try {
                                                if (dVar.a(xu4Var2) && atomicInteger.get() == i6 && !vs4.L(str15)) {
                                                    try {
                                                        up3Var = up3Var3;
                                                        try {
                                                            str10 = str11;
                                                            try {
                                                                JSONObject jSONObjectT = ql3.t(new File(up3Var.d, m54.a("s1:I97YlxmP0Pd2WWs3h5dn0AxTwotPGYJ+b4jrHiSgosT1+TEqKprsOn7uQcoQ0A==")).getPath(), str15, str16, str10);
                                                                j04Var = j04Var4;
                                                                try {
                                                                    if (j04Var.g == null) {
                                                                        j04Var.g = jSONObjectT.getString("id");
                                                                    }
                                                                } catch (Exception unused) {
                                                                    try {
                                                                        qi qiVar9 = qi.a;
                                                                        qi.e(m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="), m54.a("s1:OJ6Aj28oNB36Z15edERM4mIRMVxdPL6wczf/9/U19i0aMdjHBlNsAWBoYfaeWJ9Xrxrwsya5UFEAZI6N1UG9ZZzRbETY9hgsYWPvtVljCzNuok3J/zjjIiq1edDrcw=="));
                                                                    } catch (Exception e12) {
                                                                        e = e12;
                                                                        i04Var = i04Var;
                                                                        f04Var2 = f04Var;
                                                                        vh3Var2 = vh3Var2;
                                                                        xu4Var2 = xu4Var2;
                                                                        j4 = j3;
                                                                        j04Var2 = j04Var5;
                                                                        j04Var3 = j04Var6;
                                                                        up3Var3 = up3Var;
                                                                        i2 = i6;
                                                                        str16 = str16;
                                                                        qi qiVar10 = qi.a;
                                                                        y82.t("s1:6auriwWRmrn0CZ9kkxwjBwSVQPCAfrKN3FM6ckcoDWxmSAgXTmn40eBlz/L5ZbvT9SinBw==", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="));
                                                                        Handler handler8 = up3Var3.R0;
                                                                        final String str1112 = str6;
                                                                        final i04 i04Var10 = i04Var;
                                                                        final j04 j04Var1111 = j04Var;
                                                                        final String str1113 = str16;
                                                                        final j04 j04Var1112 = j04Var2;
                                                                        final String str27 = str10;
                                                                        final j04 j04Var1113 = j04Var3;
                                                                        final up3 up3Var11 = up3Var3;
                                                                        final boolean z19 = z4;
                                                                        handler8.post(new Runnable() { // from class: yo3
                                                                            @Override // java.lang.Runnable
                                                                            public final void run() throws JSONException {
                                                                                Map mapSingletonMap;
                                                                                up3 up3Var12 = up3Var11;
                                                                                boolean z110 = up3Var12.V0;
                                                                                long j10 = j4;
                                                                                boolean z111 = false;
                                                                                boolean z112 = !z110 && up3Var12.U0.get() == i2 && up3Var12.K().a == j10;
                                                                                boolean z113 = z112 && up3Var12.T.a(xu4Var2);
                                                                                i04 i04Var11 = i04Var10;
                                                                                f04 f04Var3 = f04Var2;
                                                                                if (z112 && i04Var11.g > 0) {
                                                                                    LinkedHashMap linkedHashMap = up3Var12.U;
                                                                                    String str28 = str1113;
                                                                                    linkedHashMap.put(str28, str27);
                                                                                    String str29 = str1112;
                                                                                    if (str29 != null) {
                                                                                        up3Var12.V.put(str28, str29);
                                                                                    }
                                                                                    String str210 = (String) j04Var1111.g;
                                                                                    if (str210 != null) {
                                                                                        up3Var12.W.put(str28, str210);
                                                                                    }
                                                                                    Map map = up3Var12.a0;
                                                                                    Long lValueOf2 = Long.valueOf(i04Var11.g);
                                                                                    map.getClass();
                                                                                    if (map.isEmpty()) {
                                                                                        mapSingletonMap = Collections.singletonMap(lValueOf2, str28);
                                                                                        mapSingletonMap.getClass();
                                                                                    } else {
                                                                                        LinkedHashMap linkedHashMap2 = new LinkedHashMap(map);
                                                                                        linkedHashMap2.put(lValueOf2, str28);
                                                                                        mapSingletonMap = linkedHashMap2;
                                                                                    }
                                                                                    up3Var12.a0 = mapSingletonMap;
                                                                                    if (z113 && f04Var3.g) {
                                                                                        JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")).put("id", i04Var11.g).put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str28);
                                                                                        m54.a("s1:uZf+OdyW6A/yp2ivdNbkL8GlGcQ0NfqxfMkPw9IFXWpL70RILNRmRaL9gYc=");
                                                                                        jSONObjectPut.getClass();
                                                                                        up3Var12.a(vh3Var2, jSONObjectPut, z19);
                                                                                    }
                                                                                    if (z113) {
                                                                                        j04 j04Var1114 = j04Var1112;
                                                                                        sh3 sh3Var5 = (sh3) j04Var1114.g;
                                                                                        if (sh3Var5 != null && sh3Var5.a == up3Var12.K().a) {
                                                                                            Object obj5 = j04Var1114.g;
                                                                                            obj5.getClass();
                                                                                            ArrayList arrayList5 = ((sh3) obj5).b;
                                                                                            m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                                                            arrayList5.getClass();
                                                                                            up3Var12.f0.setValue(b60.G1(arrayList5));
                                                                                            up3Var12.o0.setValue((List) j04Var1113.g);
                                                                                        }
                                                                                    }
                                                                                    up3Var12.b0();
                                                                                }
                                                                                qi qiVar11 = qi.a;
                                                                                String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                                                                                boolean z114 = i04Var11.g > 0;
                                                                                boolean z115 = f04Var3.g && z113;
                                                                                StringBuilder sb = new StringBuilder();
                                                                                y82.s("s1:MYwDiD84uurrZSf/89T7hJNNjUM73I97ryP9oY4Zsq82Yx+sQQim2Ma4GZfh60xGfqwEe8kYRyXOGUSAdrfCRJrS/w5bMzzJt/kYfS9VjpAnWSpnRTg=", sb, j10, "s1:F99ebhDddbJBUBFAXCX5cOF01dNn9bJSwAtEgI/6MOocGCCdbKfKvmrRX8dxVZbRMBRxRw==");
                                                                                sb.append(z114);
                                                                                sb.append(m54.a("s1:WbnpR8kLfKtfNT/YlYJcbQgdcN0sq1qBaaQOBhJDvj30/uT8/bPObj+rhrQyTmMa"));
                                                                                sb.append(z115);
                                                                                sb.append(m54.a("s1:/n/N7UpFZSMSeu0Wlmd1rQaMl/+uEWBUxf0hUW/fTCZb7V9KyWX4z0jWZFZhD88PlAPj5g=="));
                                                                                sb.append(!z113);
                                                                                qi.a(strA, sb.toString());
                                                                                long j11 = i04Var11.g;
                                                                                if (f04Var3.g && z113) {
                                                                                    z111 = true;
                                                                                }
                                                                                completableFuture2.complete(new sp3(j11, z111));
                                                                            }
                                                                        });
                                                                        up3Var3.v1 = 0;
                                                                        return;
                                                                    }
                                                                }
                                                            } catch (Exception unused2) {
                                                                j04Var = j04Var4;
                                                                qi qiVar11 = qi.a;
                                                                qi.e(m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="), m54.a("s1:OJ6Aj28oNB36Z15edERM4mIRMVxdPL6wczf/9/U19i0aMdjHBlNsAWBoYfaeWJ9Xrxrwsya5UFEAZI6N1UG9ZZzRbETY9hgsYWPvtVljCzNuok3J/zjjIiq1edDrcw=="));
                                                                if (f04Var.g) {
                                                                    i04Var = i04Var;
                                                                    f04Var2 = f04Var;
                                                                    vh3Var2 = vh3Var2;
                                                                    xu4Var2 = xu4Var2;
                                                                    j4 = j3;
                                                                    up3Var3 = up3Var;
                                                                    i2 = i6;
                                                                    str16 = str16;
                                                                    if (i04Var.g > 0) {
                                                                        Object obj5 = player.t().get();
                                                                        j04Var2 = j04Var5;
                                                                        j04Var2.g = obj5;
                                                                        sh3Var = (sh3) obj5;
                                                                        if (sh3Var != null) {
                                                                            DateTimeFormatter dateTimeFormatter3 = vp2.a;
                                                                            objE = vp2.e(sh3Var, (aq2) up3Var3.r0.getValue(), up3Var3.m0);
                                                                        } else {
                                                                            objE = null;
                                                                        }
                                                                        if (objE == null) {
                                                                            objE = sz0Var;
                                                                        }
                                                                        j04Var3 = j04Var6;
                                                                        j04Var3.g = objE;
                                                                    } else {
                                                                        j04Var2 = j04Var5;
                                                                        j04Var3 = j04Var6;
                                                                    }
                                                                } else {
                                                                    i04Var = i04Var;
                                                                    f04Var2 = f04Var;
                                                                    vh3Var2 = vh3Var2;
                                                                    xu4Var2 = xu4Var2;
                                                                    j4 = j3;
                                                                    up3Var3 = up3Var;
                                                                    i2 = i6;
                                                                    str16 = str16;
                                                                    if (i04Var.g > 0) {
                                                                        Object obj6 = player.t().get();
                                                                        j04Var2 = j04Var5;
                                                                        j04Var2.g = obj6;
                                                                        sh3Var = (sh3) obj6;
                                                                        if (sh3Var != null) {
                                                                            DateTimeFormatter dateTimeFormatter4 = vp2.a;
                                                                            objE = vp2.e(sh3Var, (aq2) up3Var3.r0.getValue(), up3Var3.m0);
                                                                        } else {
                                                                            objE = null;
                                                                        }
                                                                        if (objE == null) {
                                                                            objE = sz0Var;
                                                                        }
                                                                        j04Var3 = j04Var6;
                                                                        j04Var3.g = objE;
                                                                    } else {
                                                                        j04Var2 = j04Var5;
                                                                        j04Var3 = j04Var6;
                                                                    }
                                                                }
                                                                Handler handler9 = up3Var3.R0;
                                                                final String str1114 = str6;
                                                                final i04 i04Var11 = i04Var;
                                                                final j04 j04Var1114 = j04Var;
                                                                final String str1115 = str16;
                                                                final j04 j04Var1115 = j04Var2;
                                                                final String str28 = str10;
                                                                final j04 j04Var1116 = j04Var3;
                                                                final up3 up3Var12 = up3Var3;
                                                                final boolean z110 = z4;
                                                                handler9.post(new Runnable() { // from class: yo3
                                                                    @Override // java.lang.Runnable
                                                                    public final void run() throws JSONException {
                                                                        Map mapSingletonMap;
                                                                        up3 up3Var13 = up3Var12;
                                                                        boolean z111 = up3Var13.V0;
                                                                        long j10 = j4;
                                                                        boolean z112 = false;
                                                                        boolean z113 = !z111 && up3Var13.U0.get() == i2 && up3Var13.K().a == j10;
                                                                        boolean z114 = z113 && up3Var13.T.a(xu4Var2);
                                                                        i04 i04Var12 = i04Var11;
                                                                        f04 f04Var3 = f04Var2;
                                                                        if (z113 && i04Var12.g > 0) {
                                                                            LinkedHashMap linkedHashMap = up3Var13.U;
                                                                            String str29 = str1115;
                                                                            linkedHashMap.put(str29, str28);
                                                                            String str210 = str1114;
                                                                            if (str210 != null) {
                                                                                up3Var13.V.put(str29, str210);
                                                                            }
                                                                            String str211 = (String) j04Var1114.g;
                                                                            if (str211 != null) {
                                                                                up3Var13.W.put(str29, str211);
                                                                            }
                                                                            Map map = up3Var13.a0;
                                                                            Long lValueOf2 = Long.valueOf(i04Var12.g);
                                                                            map.getClass();
                                                                            if (map.isEmpty()) {
                                                                                mapSingletonMap = Collections.singletonMap(lValueOf2, str29);
                                                                                mapSingletonMap.getClass();
                                                                            } else {
                                                                                LinkedHashMap linkedHashMap2 = new LinkedHashMap(map);
                                                                                linkedHashMap2.put(lValueOf2, str29);
                                                                                mapSingletonMap = linkedHashMap2;
                                                                            }
                                                                            up3Var13.a0 = mapSingletonMap;
                                                                            if (z114 && f04Var3.g) {
                                                                                JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")).put("id", i04Var12.g).put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str29);
                                                                                m54.a("s1:uZf+OdyW6A/yp2ivdNbkL8GlGcQ0NfqxfMkPw9IFXWpL70RILNRmRaL9gYc=");
                                                                                jSONObjectPut.getClass();
                                                                                up3Var13.a(vh3Var2, jSONObjectPut, z110);
                                                                            }
                                                                            if (z114) {
                                                                                j04 j04Var1117 = j04Var1115;
                                                                                sh3 sh3Var5 = (sh3) j04Var1117.g;
                                                                                if (sh3Var5 != null && sh3Var5.a == up3Var13.K().a) {
                                                                                    Object obj7 = j04Var1117.g;
                                                                                    obj7.getClass();
                                                                                    ArrayList arrayList5 = ((sh3) obj7).b;
                                                                                    m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                                                    arrayList5.getClass();
                                                                                    up3Var13.f0.setValue(b60.G1(arrayList5));
                                                                                    up3Var13.o0.setValue((List) j04Var1116.g);
                                                                                }
                                                                            }
                                                                            up3Var13.b0();
                                                                        }
                                                                        qi qiVar12 = qi.a;
                                                                        String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                                                                        boolean z115 = i04Var12.g > 0;
                                                                        boolean z116 = f04Var3.g && z114;
                                                                        StringBuilder sb = new StringBuilder();
                                                                        y82.s("s1:MYwDiD84uurrZSf/89T7hJNNjUM73I97ryP9oY4Zsq82Yx+sQQim2Ma4GZfh60xGfqwEe8kYRyXOGUSAdrfCRJrS/w5bMzzJt/kYfS9VjpAnWSpnRTg=", sb, j10, "s1:F99ebhDddbJBUBFAXCX5cOF01dNn9bJSwAtEgI/6MOocGCCdbKfKvmrRX8dxVZbRMBRxRw==");
                                                                        sb.append(z115);
                                                                        sb.append(m54.a("s1:WbnpR8kLfKtfNT/YlYJcbQgdcN0sq1qBaaQOBhJDvj30/uT8/bPObj+rhrQyTmMa"));
                                                                        sb.append(z116);
                                                                        sb.append(m54.a("s1:/n/N7UpFZSMSeu0Wlmd1rQaMl/+uEWBUxf0hUW/fTCZb7V9KyWX4z0jWZFZhD88PlAPj5g=="));
                                                                        sb.append(!z114);
                                                                        qi.a(strA, sb.toString());
                                                                        long j11 = i04Var12.g;
                                                                        if (f04Var3.g && z114) {
                                                                            z112 = true;
                                                                        }
                                                                        completableFuture2.complete(new sp3(j11, z112));
                                                                    }
                                                                });
                                                                up3Var3.v1 = 0;
                                                                return;
                                                            }
                                                        } catch (Exception unused3) {
                                                            str10 = str11;
                                                        }
                                                    } catch (Exception unused4) {
                                                        str10 = str11;
                                                        up3Var = up3Var3;
                                                    }
                                                } else {
                                                    str10 = str11;
                                                    up3Var = up3Var3;
                                                    j04Var = j04Var4;
                                                }
                                                if (f04Var.g) {
                                                    i04Var = i04Var;
                                                    f04Var2 = f04Var;
                                                    vh3Var2 = vh3Var2;
                                                    xu4Var2 = xu4Var2;
                                                    j4 = j3;
                                                    up3Var3 = up3Var;
                                                    i2 = i6;
                                                    str16 = str16;
                                                    if (i04Var.g > 0) {
                                                        Object obj7 = player.t().get();
                                                        j04Var2 = j04Var5;
                                                        j04Var2.g = obj7;
                                                        sh3Var = (sh3) obj7;
                                                        if (sh3Var != null) {
                                                            DateTimeFormatter dateTimeFormatter5 = vp2.a;
                                                            objE = vp2.e(sh3Var, (aq2) up3Var3.r0.getValue(), up3Var3.m0);
                                                        } else {
                                                            objE = null;
                                                        }
                                                        if (objE == null) {
                                                            objE = sz0Var;
                                                        }
                                                        j04Var3 = j04Var6;
                                                        j04Var3.g = objE;
                                                    } else {
                                                        j04Var2 = j04Var5;
                                                        j04Var3 = j04Var6;
                                                    }
                                                } else {
                                                    i04Var = i04Var;
                                                    f04Var2 = f04Var;
                                                    vh3Var2 = vh3Var2;
                                                    xu4Var2 = xu4Var2;
                                                    j4 = j3;
                                                    up3Var3 = up3Var;
                                                    i2 = i6;
                                                    str16 = str16;
                                                    if (i04Var.g > 0) {
                                                        Object obj8 = player.t().get();
                                                        j04Var2 = j04Var5;
                                                        j04Var2.g = obj8;
                                                        sh3Var = (sh3) obj8;
                                                        if (sh3Var != null) {
                                                            DateTimeFormatter dateTimeFormatter6 = vp2.a;
                                                            objE = vp2.e(sh3Var, (aq2) up3Var3.r0.getValue(), up3Var3.m0);
                                                        } else {
                                                            objE = null;
                                                        }
                                                        if (objE == null) {
                                                            objE = sz0Var;
                                                        }
                                                        j04Var3 = j04Var6;
                                                        j04Var3.g = objE;
                                                    } else {
                                                        j04Var2 = j04Var5;
                                                        j04Var3 = j04Var6;
                                                    }
                                                }
                                            } catch (Exception e13) {
                                                e = e13;
                                                str10 = str11;
                                                up3Var = up3Var3;
                                                j04Var = j04Var4;
                                                i04Var = i04Var;
                                                f04Var2 = f04Var;
                                                vh3Var2 = vh3Var2;
                                                xu4Var2 = xu4Var2;
                                                j4 = j3;
                                                j04Var2 = j04Var5;
                                                j04Var3 = j04Var6;
                                                up3Var3 = up3Var;
                                                i2 = i6;
                                                str16 = str16;
                                                qi qiVar12 = qi.a;
                                                y82.t("s1:6auriwWRmrn0CZ9kkxwjBwSVQPCAfrKN3FM6ckcoDWxmSAgXTmn40eBlz/L5ZbvT9SinBw==", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="));
                                                Handler handler10 = up3Var3.R0;
                                                final String str1116 = str6;
                                                final i04 i04Var12 = i04Var;
                                                final j04 j04Var1117 = j04Var;
                                                final String str1117 = str16;
                                                final j04 j04Var1118 = j04Var2;
                                                final String str29 = str10;
                                                final j04 j04Var1119 = j04Var3;
                                                final up3 up3Var13 = up3Var3;
                                                final boolean z111 = z4;
                                                handler10.post(new Runnable() { // from class: yo3
                                                    @Override // java.lang.Runnable
                                                    public final void run() throws JSONException {
                                                        Map mapSingletonMap;
                                                        up3 up3Var14 = up3Var13;
                                                        boolean z112 = up3Var14.V0;
                                                        long j10 = j4;
                                                        boolean z113 = false;
                                                        boolean z114 = !z112 && up3Var14.U0.get() == i2 && up3Var14.K().a == j10;
                                                        boolean z115 = z114 && up3Var14.T.a(xu4Var2);
                                                        i04 i04Var13 = i04Var12;
                                                        f04 f04Var3 = f04Var2;
                                                        if (z114 && i04Var13.g > 0) {
                                                            LinkedHashMap linkedHashMap = up3Var14.U;
                                                            String str210 = str1117;
                                                            linkedHashMap.put(str210, str29);
                                                            String str211 = str1116;
                                                            if (str211 != null) {
                                                                up3Var14.V.put(str210, str211);
                                                            }
                                                            String str212 = (String) j04Var1117.g;
                                                            if (str212 != null) {
                                                                up3Var14.W.put(str210, str212);
                                                            }
                                                            Map map = up3Var14.a0;
                                                            Long lValueOf2 = Long.valueOf(i04Var13.g);
                                                            map.getClass();
                                                            if (map.isEmpty()) {
                                                                mapSingletonMap = Collections.singletonMap(lValueOf2, str210);
                                                                mapSingletonMap.getClass();
                                                            } else {
                                                                LinkedHashMap linkedHashMap2 = new LinkedHashMap(map);
                                                                linkedHashMap2.put(lValueOf2, str210);
                                                                mapSingletonMap = linkedHashMap2;
                                                            }
                                                            up3Var14.a0 = mapSingletonMap;
                                                            if (z115 && f04Var3.g) {
                                                                JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")).put("id", i04Var13.g).put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str210);
                                                                m54.a("s1:uZf+OdyW6A/yp2ivdNbkL8GlGcQ0NfqxfMkPw9IFXWpL70RILNRmRaL9gYc=");
                                                                jSONObjectPut.getClass();
                                                                up3Var14.a(vh3Var2, jSONObjectPut, z111);
                                                            }
                                                            if (z115) {
                                                                j04 j04Var11110 = j04Var1118;
                                                                sh3 sh3Var5 = (sh3) j04Var11110.g;
                                                                if (sh3Var5 != null && sh3Var5.a == up3Var14.K().a) {
                                                                    Object obj9 = j04Var11110.g;
                                                                    obj9.getClass();
                                                                    ArrayList arrayList5 = ((sh3) obj9).b;
                                                                    m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                                    arrayList5.getClass();
                                                                    up3Var14.f0.setValue(b60.G1(arrayList5));
                                                                    up3Var14.o0.setValue((List) j04Var1119.g);
                                                                }
                                                            }
                                                            up3Var14.b0();
                                                        }
                                                        qi qiVar13 = qi.a;
                                                        String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                                                        boolean z116 = i04Var13.g > 0;
                                                        boolean z117 = f04Var3.g && z115;
                                                        StringBuilder sb = new StringBuilder();
                                                        y82.s("s1:MYwDiD84uurrZSf/89T7hJNNjUM73I97ryP9oY4Zsq82Yx+sQQim2Ma4GZfh60xGfqwEe8kYRyXOGUSAdrfCRJrS/w5bMzzJt/kYfS9VjpAnWSpnRTg=", sb, j10, "s1:F99ebhDddbJBUBFAXCX5cOF01dNn9bJSwAtEgI/6MOocGCCdbKfKvmrRX8dxVZbRMBRxRw==");
                                                        sb.append(z116);
                                                        sb.append(m54.a("s1:WbnpR8kLfKtfNT/YlYJcbQgdcN0sq1qBaaQOBhJDvj30/uT8/bPObj+rhrQyTmMa"));
                                                        sb.append(z117);
                                                        sb.append(m54.a("s1:/n/N7UpFZSMSeu0Wlmd1rQaMl/+uEWBUxf0hUW/fTCZb7V9KyWX4z0jWZFZhD88PlAPj5g=="));
                                                        sb.append(!z115);
                                                        qi.a(strA, sb.toString());
                                                        long j11 = i04Var13.g;
                                                        if (f04Var3.g && z115) {
                                                            z113 = true;
                                                        }
                                                        completableFuture2.complete(new sp3(j11, z113));
                                                    }
                                                });
                                                up3Var3.v1 = 0;
                                                return;
                                            }
                                        }
                                    } catch (Exception e14) {
                                        e = e14;
                                        i04Var = i04Var;
                                        j4 = j3;
                                        str10 = str11;
                                        j04Var = j04Var4;
                                        j04Var2 = j04Var5;
                                        j04Var3 = j04Var6;
                                        f04Var2 = f04Var;
                                        i2 = i6;
                                        qi qiVar13 = qi.a;
                                        y82.t("s1:6auriwWRmrn0CZ9kkxwjBwSVQPCAfrKN3FM6ckcoDWxmSAgXTmn40eBlz/L5ZbvT9SinBw==", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="));
                                        Handler handler11 = up3Var3.R0;
                                        final String str1118 = str6;
                                        final i04 i04Var13 = i04Var;
                                        final j04 j04Var11110 = j04Var;
                                        final String str1119 = str16;
                                        final j04 j04Var11111 = j04Var2;
                                        final String str210 = str10;
                                        final j04 j04Var11112 = j04Var3;
                                        final up3 up3Var14 = up3Var3;
                                        final boolean z112 = z4;
                                        handler11.post(new Runnable() { // from class: yo3
                                            @Override // java.lang.Runnable
                                            public final void run() throws JSONException {
                                                Map mapSingletonMap;
                                                up3 up3Var15 = up3Var14;
                                                boolean z113 = up3Var15.V0;
                                                long j10 = j4;
                                                boolean z114 = false;
                                                boolean z115 = !z113 && up3Var15.U0.get() == i2 && up3Var15.K().a == j10;
                                                boolean z116 = z115 && up3Var15.T.a(xu4Var2);
                                                i04 i04Var14 = i04Var13;
                                                f04 f04Var3 = f04Var2;
                                                if (z115 && i04Var14.g > 0) {
                                                    LinkedHashMap linkedHashMap = up3Var15.U;
                                                    String str211 = str1119;
                                                    linkedHashMap.put(str211, str210);
                                                    String str212 = str1118;
                                                    if (str212 != null) {
                                                        up3Var15.V.put(str211, str212);
                                                    }
                                                    String str213 = (String) j04Var11110.g;
                                                    if (str213 != null) {
                                                        up3Var15.W.put(str211, str213);
                                                    }
                                                    Map map = up3Var15.a0;
                                                    Long lValueOf2 = Long.valueOf(i04Var14.g);
                                                    map.getClass();
                                                    if (map.isEmpty()) {
                                                        mapSingletonMap = Collections.singletonMap(lValueOf2, str211);
                                                        mapSingletonMap.getClass();
                                                    } else {
                                                        LinkedHashMap linkedHashMap2 = new LinkedHashMap(map);
                                                        linkedHashMap2.put(lValueOf2, str211);
                                                        mapSingletonMap = linkedHashMap2;
                                                    }
                                                    up3Var15.a0 = mapSingletonMap;
                                                    if (z116 && f04Var3.g) {
                                                        JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")).put("id", i04Var14.g).put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str211);
                                                        m54.a("s1:uZf+OdyW6A/yp2ivdNbkL8GlGcQ0NfqxfMkPw9IFXWpL70RILNRmRaL9gYc=");
                                                        jSONObjectPut.getClass();
                                                        up3Var15.a(vh3Var2, jSONObjectPut, z112);
                                                    }
                                                    if (z116) {
                                                        j04 j04Var11113 = j04Var11111;
                                                        sh3 sh3Var5 = (sh3) j04Var11113.g;
                                                        if (sh3Var5 != null && sh3Var5.a == up3Var15.K().a) {
                                                            Object obj9 = j04Var11113.g;
                                                            obj9.getClass();
                                                            ArrayList arrayList5 = ((sh3) obj9).b;
                                                            m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                            arrayList5.getClass();
                                                            up3Var15.f0.setValue(b60.G1(arrayList5));
                                                            up3Var15.o0.setValue((List) j04Var11112.g);
                                                        }
                                                    }
                                                    up3Var15.b0();
                                                }
                                                qi qiVar14 = qi.a;
                                                String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                                                boolean z117 = i04Var14.g > 0;
                                                boolean z118 = f04Var3.g && z116;
                                                StringBuilder sb = new StringBuilder();
                                                y82.s("s1:MYwDiD84uurrZSf/89T7hJNNjUM73I97ryP9oY4Zsq82Yx+sQQim2Ma4GZfh60xGfqwEe8kYRyXOGUSAdrfCRJrS/w5bMzzJt/kYfS9VjpAnWSpnRTg=", sb, j10, "s1:F99ebhDddbJBUBFAXCX5cOF01dNn9bJSwAtEgI/6MOocGCCdbKfKvmrRX8dxVZbRMBRxRw==");
                                                sb.append(z117);
                                                sb.append(m54.a("s1:WbnpR8kLfKtfNT/YlYJcbQgdcN0sq1qBaaQOBhJDvj30/uT8/bPObj+rhrQyTmMa"));
                                                sb.append(z118);
                                                sb.append(m54.a("s1:/n/N7UpFZSMSeu0Wlmd1rQaMl/+uEWBUxf0hUW/fTCZb7V9KyWX4z0jWZFZhD88PlAPj5g=="));
                                                sb.append(!z116);
                                                qi.a(strA, sb.toString());
                                                long j11 = i04Var14.g;
                                                if (f04Var3.g && z116) {
                                                    z114 = true;
                                                }
                                                completableFuture2.complete(new sp3(j11, z114));
                                            }
                                        });
                                        up3Var3.v1 = 0;
                                        return;
                                    }
                                } catch (Exception e15) {
                                    e = e15;
                                    str11 = str17;
                                    f04Var = f04Var2;
                                    z4 = z10;
                                    j04Var4 = j04Var7;
                                    j04Var5 = j04Var8;
                                    j04Var6 = j04Var9;
                                    i04Var = i04Var2;
                                    i04Var = i04Var;
                                    j4 = j3;
                                    str10 = str11;
                                    j04Var = j04Var4;
                                    j04Var2 = j04Var5;
                                    j04Var3 = j04Var6;
                                    f04Var2 = f04Var;
                                    i2 = i6;
                                    qi qiVar14 = qi.a;
                                    y82.t("s1:6auriwWRmrn0CZ9kkxwjBwSVQPCAfrKN3FM6ckcoDWxmSAgXTmn40eBlz/L5ZbvT9SinBw==", new StringBuilder(), e.getClass().getSimpleName(), m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="));
                                    Handler handler12 = up3Var3.R0;
                                    final String str11110 = str6;
                                    final i04 i04Var14 = i04Var;
                                    final j04 j04Var11113 = j04Var;
                                    final String str11111 = str16;
                                    final j04 j04Var11114 = j04Var2;
                                    final String str211 = str10;
                                    final j04 j04Var11115 = j04Var3;
                                    final up3 up3Var15 = up3Var3;
                                    final boolean z113 = z4;
                                    handler12.post(new Runnable() { // from class: yo3
                                        @Override // java.lang.Runnable
                                        public final void run() throws JSONException {
                                            Map mapSingletonMap;
                                            up3 up3Var16 = up3Var15;
                                            boolean z114 = up3Var16.V0;
                                            long j10 = j4;
                                            boolean z115 = false;
                                            boolean z116 = !z114 && up3Var16.U0.get() == i2 && up3Var16.K().a == j10;
                                            boolean z117 = z116 && up3Var16.T.a(xu4Var2);
                                            i04 i04Var15 = i04Var14;
                                            f04 f04Var3 = f04Var2;
                                            if (z116 && i04Var15.g > 0) {
                                                LinkedHashMap linkedHashMap = up3Var16.U;
                                                String str212 = str11111;
                                                linkedHashMap.put(str212, str211);
                                                String str213 = str11110;
                                                if (str213 != null) {
                                                    up3Var16.V.put(str212, str213);
                                                }
                                                String str214 = (String) j04Var11113.g;
                                                if (str214 != null) {
                                                    up3Var16.W.put(str212, str214);
                                                }
                                                Map map = up3Var16.a0;
                                                Long lValueOf2 = Long.valueOf(i04Var15.g);
                                                map.getClass();
                                                if (map.isEmpty()) {
                                                    mapSingletonMap = Collections.singletonMap(lValueOf2, str212);
                                                    mapSingletonMap.getClass();
                                                } else {
                                                    LinkedHashMap linkedHashMap2 = new LinkedHashMap(map);
                                                    linkedHashMap2.put(lValueOf2, str212);
                                                    mapSingletonMap = linkedHashMap2;
                                                }
                                                up3Var16.a0 = mapSingletonMap;
                                                if (z117 && f04Var3.g) {
                                                    JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")).put("id", i04Var15.g).put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str212);
                                                    m54.a("s1:uZf+OdyW6A/yp2ivdNbkL8GlGcQ0NfqxfMkPw9IFXWpL70RILNRmRaL9gYc=");
                                                    jSONObjectPut.getClass();
                                                    up3Var16.a(vh3Var2, jSONObjectPut, z113);
                                                }
                                                if (z117) {
                                                    j04 j04Var11116 = j04Var11114;
                                                    sh3 sh3Var5 = (sh3) j04Var11116.g;
                                                    if (sh3Var5 != null && sh3Var5.a == up3Var16.K().a) {
                                                        Object obj9 = j04Var11116.g;
                                                        obj9.getClass();
                                                        ArrayList arrayList5 = ((sh3) obj9).b;
                                                        m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                        arrayList5.getClass();
                                                        up3Var16.f0.setValue(b60.G1(arrayList5));
                                                        up3Var16.o0.setValue((List) j04Var11115.g);
                                                    }
                                                }
                                                up3Var16.b0();
                                            }
                                            qi qiVar15 = qi.a;
                                            String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                                            boolean z118 = i04Var15.g > 0;
                                            boolean z119 = f04Var3.g && z117;
                                            StringBuilder sb = new StringBuilder();
                                            y82.s("s1:MYwDiD84uurrZSf/89T7hJNNjUM73I97ryP9oY4Zsq82Yx+sQQim2Ma4GZfh60xGfqwEe8kYRyXOGUSAdrfCRJrS/w5bMzzJt/kYfS9VjpAnWSpnRTg=", sb, j10, "s1:F99ebhDddbJBUBFAXCX5cOF01dNn9bJSwAtEgI/6MOocGCCdbKfKvmrRX8dxVZbRMBRxRw==");
                                            sb.append(z118);
                                            sb.append(m54.a("s1:WbnpR8kLfKtfNT/YlYJcbQgdcN0sq1qBaaQOBhJDvj30/uT8/bPObj+rhrQyTmMa"));
                                            sb.append(z119);
                                            sb.append(m54.a("s1:/n/N7UpFZSMSeu0Wlmd1rQaMl/+uEWBUxf0hUW/fTCZb7V9KyWX4z0jWZFZhD88PlAPj5g=="));
                                            sb.append(!z117);
                                            qi.a(strA, sb.toString());
                                            long j11 = i04Var15.g;
                                            if (f04Var3.g && z117) {
                                                z115 = true;
                                            }
                                            completableFuture2.complete(new sp3(j11, z115));
                                        }
                                    });
                                    up3Var3.v1 = 0;
                                    return;
                                }
                                Handler handler13 = up3Var3.R0;
                                final String str11112 = str6;
                                final i04 i04Var15 = i04Var;
                                final j04 j04Var11116 = j04Var;
                                final String str11113 = str16;
                                final j04 j04Var11117 = j04Var2;
                                final String str212 = str10;
                                final j04 j04Var11118 = j04Var3;
                                final up3 up3Var16 = up3Var3;
                                final boolean z114 = z4;
                                handler13.post(new Runnable() { // from class: yo3
                                    @Override // java.lang.Runnable
                                    public final void run() throws JSONException {
                                        Map mapSingletonMap;
                                        up3 up3Var17 = up3Var16;
                                        boolean z115 = up3Var17.V0;
                                        long j10 = j4;
                                        boolean z116 = false;
                                        boolean z117 = !z115 && up3Var17.U0.get() == i2 && up3Var17.K().a == j10;
                                        boolean z118 = z117 && up3Var17.T.a(xu4Var2);
                                        i04 i04Var16 = i04Var15;
                                        f04 f04Var3 = f04Var2;
                                        if (z117 && i04Var16.g > 0) {
                                            LinkedHashMap linkedHashMap = up3Var17.U;
                                            String str213 = str11113;
                                            linkedHashMap.put(str213, str212);
                                            String str214 = str11112;
                                            if (str214 != null) {
                                                up3Var17.V.put(str213, str214);
                                            }
                                            String str215 = (String) j04Var11116.g;
                                            if (str215 != null) {
                                                up3Var17.W.put(str213, str215);
                                            }
                                            Map map = up3Var17.a0;
                                            Long lValueOf2 = Long.valueOf(i04Var16.g);
                                            map.getClass();
                                            if (map.isEmpty()) {
                                                mapSingletonMap = Collections.singletonMap(lValueOf2, str213);
                                                mapSingletonMap.getClass();
                                            } else {
                                                LinkedHashMap linkedHashMap2 = new LinkedHashMap(map);
                                                linkedHashMap2.put(lValueOf2, str213);
                                                mapSingletonMap = linkedHashMap2;
                                            }
                                            up3Var17.a0 = mapSingletonMap;
                                            if (z118 && f04Var3.g) {
                                                JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")).put("id", i04Var16.g).put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str213);
                                                m54.a("s1:uZf+OdyW6A/yp2ivdNbkL8GlGcQ0NfqxfMkPw9IFXWpL70RILNRmRaL9gYc=");
                                                jSONObjectPut.getClass();
                                                up3Var17.a(vh3Var2, jSONObjectPut, z114);
                                            }
                                            if (z118) {
                                                j04 j04Var11119 = j04Var11117;
                                                sh3 sh3Var5 = (sh3) j04Var11119.g;
                                                if (sh3Var5 != null && sh3Var5.a == up3Var17.K().a) {
                                                    Object obj9 = j04Var11119.g;
                                                    obj9.getClass();
                                                    ArrayList arrayList5 = ((sh3) obj9).b;
                                                    m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                    arrayList5.getClass();
                                                    up3Var17.f0.setValue(b60.G1(arrayList5));
                                                    up3Var17.o0.setValue((List) j04Var11118.g);
                                                }
                                            }
                                            up3Var17.b0();
                                        }
                                        qi qiVar15 = qi.a;
                                        String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                                        boolean z119 = i04Var16.g > 0;
                                        boolean z1110 = f04Var3.g && z118;
                                        StringBuilder sb = new StringBuilder();
                                        y82.s("s1:MYwDiD84uurrZSf/89T7hJNNjUM73I97ryP9oY4Zsq82Yx+sQQim2Ma4GZfh60xGfqwEe8kYRyXOGUSAdrfCRJrS/w5bMzzJt/kYfS9VjpAnWSpnRTg=", sb, j10, "s1:F99ebhDddbJBUBFAXCX5cOF01dNn9bJSwAtEgI/6MOocGCCdbKfKvmrRX8dxVZbRMBRxRw==");
                                        sb.append(z119);
                                        sb.append(m54.a("s1:WbnpR8kLfKtfNT/YlYJcbQgdcN0sq1qBaaQOBhJDvj30/uT8/bPObj+rhrQyTmMa"));
                                        sb.append(z1110);
                                        sb.append(m54.a("s1:/n/N7UpFZSMSeu0Wlmd1rQaMl/+uEWBUxf0hUW/fTCZb7V9KyWX4z0jWZFZhD88PlAPj5g=="));
                                        sb.append(!z118);
                                        qi.a(strA, sb.toString());
                                        long j11 = i04Var16.g;
                                        if (f04Var3.g && z118) {
                                            z116 = true;
                                        }
                                        completableFuture2.complete(new sp3(j11, z116));
                                    }
                                });
                                up3Var3.v1 = 0;
                                return;
                            }
                            str11 = str17;
                            f04Var = f04Var2;
                            z7 = z5;
                            j04Var4 = j04Var7;
                            z4 = z10;
                            j04Var5 = j04Var8;
                            j04Var6 = j04Var9;
                            i04Var = i04Var2;
                            z8 = false;
                            r23 = z7;
                            if (i04Var.g > 0) {
                                nh3 nh3Var2 = nh3.p;
                                long j10 = i04Var.g;
                                double dOrdinal3 = vh3Var2.ordinal();
                                double dOrdinal4 = nh3Var2.ordinal();
                                double[] dArr2 = new double[3];
                                dArr2[0] = dOrdinal3;
                                dArr2[r23] = dOrdinal4;
                                dArr2[2] = j10;
                                num2 = (Integer) player.G(m54.a("s1:CZN+87AuCwWA1Ti8auQXyRW+Sr6M6iz2dO8O99X80Iq3zIMKMS+WQw=="), j3, new String[0], dArr2).get();
                                if (num2 != null) {
                                    ArrayList arrayList5 = sh3Var2.b;
                                    m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                    arrayList5.getClass();
                                    ?? r2 = gi3.B(arrayList5, vh3Var2, i04Var.g, z6).getBoolean(m54.a("s1:JG00ENWTf6nMmP5/OEFjXo82Q4hMwj9YajI10HbaT9BispSdrZNYITpghIQ=")) ? r23 : 0;
                                    f04Var.g = r2;
                                }
                                f04Var.g = r2;
                            }
                            if (f04Var.g) {
                                str10 = str11;
                                up3Var = up3Var3;
                                j04Var = j04Var4;
                                if (f04Var.g) {
                                    i04Var = i04Var;
                                    f04Var2 = f04Var;
                                    vh3Var2 = vh3Var2;
                                    xu4Var2 = xu4Var2;
                                    j4 = j3;
                                    up3Var3 = up3Var;
                                    i2 = i6;
                                    str16 = str16;
                                    if (i04Var.g > 0) {
                                        Object obj9 = player.t().get();
                                        j04Var2 = j04Var5;
                                        j04Var2.g = obj9;
                                        sh3Var = (sh3) obj9;
                                        if (sh3Var != null) {
                                            DateTimeFormatter dateTimeFormatter7 = vp2.a;
                                            objE = vp2.e(sh3Var, (aq2) up3Var3.r0.getValue(), up3Var3.m0);
                                        } else {
                                            objE = null;
                                        }
                                        if (objE == null) {
                                            objE = sz0Var;
                                        }
                                        j04Var3 = j04Var6;
                                        j04Var3.g = objE;
                                    } else {
                                        j04Var2 = j04Var5;
                                        j04Var3 = j04Var6;
                                    }
                                } else {
                                    i04Var = i04Var;
                                    f04Var2 = f04Var;
                                    vh3Var2 = vh3Var2;
                                    xu4Var2 = xu4Var2;
                                    j4 = j3;
                                    up3Var3 = up3Var;
                                    i2 = i6;
                                    str16 = str16;
                                    if (i04Var.g > 0) {
                                        Object obj10 = player.t().get();
                                        j04Var2 = j04Var5;
                                        j04Var2.g = obj10;
                                        sh3Var = (sh3) obj10;
                                        if (sh3Var != null) {
                                            DateTimeFormatter dateTimeFormatter8 = vp2.a;
                                            objE = vp2.e(sh3Var, (aq2) up3Var3.r0.getValue(), up3Var3.m0);
                                        } else {
                                            objE = null;
                                        }
                                        if (objE == null) {
                                            objE = sz0Var;
                                        }
                                        j04Var3 = j04Var6;
                                        j04Var3.g = objE;
                                    } else {
                                        j04Var2 = j04Var5;
                                        j04Var3 = j04Var6;
                                    }
                                }
                            } else {
                                str10 = str11;
                                up3Var = up3Var3;
                                j04Var = j04Var4;
                                if (f04Var.g) {
                                    i04Var = i04Var;
                                    f04Var2 = f04Var;
                                    vh3Var2 = vh3Var2;
                                    xu4Var2 = xu4Var2;
                                    j4 = j3;
                                    up3Var3 = up3Var;
                                    i2 = i6;
                                    str16 = str16;
                                    if (i04Var.g > 0) {
                                        Object obj11 = player.t().get();
                                        j04Var2 = j04Var5;
                                        j04Var2.g = obj11;
                                        sh3Var = (sh3) obj11;
                                        if (sh3Var != null) {
                                            DateTimeFormatter dateTimeFormatter9 = vp2.a;
                                            objE = vp2.e(sh3Var, (aq2) up3Var3.r0.getValue(), up3Var3.m0);
                                        } else {
                                            objE = null;
                                        }
                                        if (objE == null) {
                                            objE = sz0Var;
                                        }
                                        j04Var3 = j04Var6;
                                        j04Var3.g = objE;
                                    } else {
                                        j04Var2 = j04Var5;
                                        j04Var3 = j04Var6;
                                    }
                                } else {
                                    i04Var = i04Var;
                                    f04Var2 = f04Var;
                                    vh3Var2 = vh3Var2;
                                    xu4Var2 = xu4Var2;
                                    j4 = j3;
                                    up3Var3 = up3Var;
                                    i2 = i6;
                                    str16 = str16;
                                    if (i04Var.g > 0) {
                                        Object obj12 = player.t().get();
                                        j04Var2 = j04Var5;
                                        j04Var2.g = obj12;
                                        sh3Var = (sh3) obj12;
                                        if (sh3Var != null) {
                                            DateTimeFormatter dateTimeFormatter10 = vp2.a;
                                            objE = vp2.e(sh3Var, (aq2) up3Var3.r0.getValue(), up3Var3.m0);
                                        } else {
                                            objE = null;
                                        }
                                        if (objE == null) {
                                            objE = sz0Var;
                                        }
                                        j04Var3 = j04Var6;
                                        j04Var3.g = objE;
                                    } else {
                                        j04Var2 = j04Var5;
                                        j04Var3 = j04Var6;
                                    }
                                }
                            }
                            Handler handler14 = up3Var3.R0;
                            final String str11114 = str6;
                            final i04 i04Var16 = i04Var;
                            final j04 j04Var11119 = j04Var;
                            final String str11115 = str16;
                            final j04 j04Var111110 = j04Var2;
                            final String str213 = str10;
                            final j04 j04Var111111 = j04Var3;
                            final up3 up3Var17 = up3Var3;
                            final boolean z115 = z4;
                            handler14.post(new Runnable() { // from class: yo3
                                @Override // java.lang.Runnable
                                public final void run() throws JSONException {
                                    Map mapSingletonMap;
                                    up3 up3Var18 = up3Var17;
                                    boolean z116 = up3Var18.V0;
                                    long j11 = j4;
                                    boolean z117 = false;
                                    boolean z118 = !z116 && up3Var18.U0.get() == i2 && up3Var18.K().a == j11;
                                    boolean z119 = z118 && up3Var18.T.a(xu4Var2);
                                    i04 i04Var17 = i04Var16;
                                    f04 f04Var3 = f04Var2;
                                    if (z118 && i04Var17.g > 0) {
                                        LinkedHashMap linkedHashMap = up3Var18.U;
                                        String str214 = str11115;
                                        linkedHashMap.put(str214, str213);
                                        String str215 = str11114;
                                        if (str215 != null) {
                                            up3Var18.V.put(str214, str215);
                                        }
                                        String str216 = (String) j04Var11119.g;
                                        if (str216 != null) {
                                            up3Var18.W.put(str214, str216);
                                        }
                                        Map map = up3Var18.a0;
                                        Long lValueOf2 = Long.valueOf(i04Var17.g);
                                        map.getClass();
                                        if (map.isEmpty()) {
                                            mapSingletonMap = Collections.singletonMap(lValueOf2, str214);
                                            mapSingletonMap.getClass();
                                        } else {
                                            LinkedHashMap linkedHashMap2 = new LinkedHashMap(map);
                                            linkedHashMap2.put(lValueOf2, str214);
                                            mapSingletonMap = linkedHashMap2;
                                        }
                                        up3Var18.a0 = mapSingletonMap;
                                        if (z119 && f04Var3.g) {
                                            JSONObject jSONObjectPut = new JSONObject().put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), m54.a("s1:GXsDEcb56MSdmhWtNkGrXDND2DftMpxivN3uX5vSpjYuLatyre8=")).put("id", i04Var17.g).put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str214);
                                            m54.a("s1:uZf+OdyW6A/yp2ivdNbkL8GlGcQ0NfqxfMkPw9IFXWpL70RILNRmRaL9gYc=");
                                            jSONObjectPut.getClass();
                                            up3Var18.a(vh3Var2, jSONObjectPut, z115);
                                        }
                                        if (z119) {
                                            j04 j04Var111112 = j04Var111110;
                                            sh3 sh3Var5 = (sh3) j04Var111112.g;
                                            if (sh3Var5 != null && sh3Var5.a == up3Var18.K().a) {
                                                Object obj13 = j04Var111112.g;
                                                obj13.getClass();
                                                ArrayList arrayList6 = ((sh3) obj13).b;
                                                m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                                arrayList6.getClass();
                                                up3Var18.f0.setValue(b60.G1(arrayList6));
                                                up3Var18.o0.setValue((List) j04Var111111.g);
                                            }
                                        }
                                        up3Var18.b0();
                                    }
                                    qi qiVar15 = qi.a;
                                    String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
                                    boolean z1110 = i04Var17.g > 0;
                                    boolean z1111 = f04Var3.g && z119;
                                    StringBuilder sb = new StringBuilder();
                                    y82.s("s1:MYwDiD84uurrZSf/89T7hJNNjUM73I97ryP9oY4Zsq82Yx+sQQim2Ma4GZfh60xGfqwEe8kYRyXOGUSAdrfCRJrS/w5bMzzJt/kYfS9VjpAnWSpnRTg=", sb, j11, "s1:F99ebhDddbJBUBFAXCX5cOF01dNn9bJSwAtEgI/6MOocGCCdbKfKvmrRX8dxVZbRMBRxRw==");
                                    sb.append(z1110);
                                    sb.append(m54.a("s1:WbnpR8kLfKtfNT/YlYJcbQgdcN0sq1qBaaQOBhJDvj30/uT8/bPObj+rhrQyTmMa"));
                                    sb.append(z1111);
                                    sb.append(m54.a("s1:/n/N7UpFZSMSeu0Wlmd1rQaMl/+uEWBUxf0hUW/fTCZb7V9KyWX4z0jWZFZhD88PlAPj5g=="));
                                    sb.append(!z119);
                                    qi.a(strA, sb.toString());
                                    long j12 = i04Var17.g;
                                    if (f04Var3.g && z119) {
                                        z117 = true;
                                    }
                                    completableFuture2.complete(new sp3(j12, z117));
                                }
                            });
                            up3Var3.v1 = 0;
                            return;
                        }
                    }
                    completableFuture2.complete(new sp3());
                }
            });
        } catch (RejectedExecutionException unused) {
            completableFuture.complete(new sp3());
        }
        CompletableFuture completableFutureThenApply = completableFuture.thenApply((Function) new ev2(new wn3(7), 29));
        m54.a("s1:CJhNE87w9yxM8Dpjr2E1GnJHXFvgk9eNTf98b96pjTBTvSntg68adRAU+/c7Nr/C2T05nBjAMME=");
        completableFutureThenApply.getClass();
        return completableFutureThenApply;
    }

    public final void b0() {
        Handler handler = this.R0;
        uo3 uo3Var = this.N;
        handler.removeCallbacks(uo3Var);
        if (this.V0 || !K().k()) {
            return;
        }
        handler.postDelayed(uo3Var, 32L);
    }

    public final xu4 d(long j, vh3 vh3Var) {
        xu4 xu4VarC;
        m54.a("s1:9APGw38pIRmFFr7xugW++AnUv7+waqJhCCY5sZBpeo/SFsH7");
        vh3Var.getClass();
        if (this.V0 || j != K().a || !K().k() || vh3Var == vh3.g) {
            return null;
        }
        d dVar = this.T;
        boolean z = vh3Var == vh3.p;
        synchronized (dVar) {
            xu4VarC = dVar.c(j, z, false);
        }
        return xu4VarC;
    }

    /* JADX WARN: Type inference fix 'apply assigned field type' failed
    java.lang.UnsupportedOperationException: ArgType.getObject(), call class: class jadx.core.dex.instructions.args.ArgType$PrimitiveArg
    	at jadx.core.dex.instructions.args.ArgType.getObject(ArgType.java:596)
    	at jadx.core.dex.attributes.nodes.ClassTypeVarsAttr.getTypeVarsMapFor(ClassTypeVarsAttr.java:35)
    	at jadx.core.dex.nodes.utils.TypeUtils.replaceClassGenerics(TypeUtils.java:177)
    	at jadx.core.dex.visitors.typeinference.FixTypesVisitor.insertExplicitUseCast(FixTypesVisitor.java:397)
    	at jadx.core.dex.visitors.typeinference.FixTypesVisitor.tryFieldTypeWithNewCasts(FixTypesVisitor.java:359)
    	at jadx.core.dex.visitors.typeinference.FixTypesVisitor.applyFieldType(FixTypesVisitor.java:309)
    	at jadx.core.dex.visitors.typeinference.FixTypesVisitor.visit(FixTypesVisitor.java:94)
     */
    public final void d0(final tm1 tm1Var, final vh3 vh3Var, final long j) throws JSONException {
        pj3 pj3Var;
        Object next;
        long j2;
        Integer num;
        List listE;
        ai3 ai3VarK;
        m54.a("s1:0FucETyJGwWTXUW3QJ5ovfllv0B9PmzDheZ446zyao2GP2rnUL4Fgw==");
        m54.a("s1:9APGw38pIRmFFr7xugW++AnUv7+waqJhCCY5sZBpeo/SFsH7");
        vh3Var.getClass();
        if (j == K().a && K().k()) {
            Object obj = null;
            int i = 0;
            if (vh3Var == vh3.p && ((nh3) tm1Var.o) == nh3.p) {
                pj3 pj3Var2 = this.I;
                if (pj3Var2 == null || (ai3VarK = pj3Var2.k(0)) == null) {
                    Iterator it = N().iterator();
                    while (true) {
                        if (!it.hasNext()) {
                            next = null;
                            break;
                        }
                        next = it.next();
                        ai3 ai3Var = (ai3) next;
                        if (gy1.n(ai3Var.f, m54.a("s1:MI/KfCDPxwtR0UsTnxJV8ADHvsD+y/XljwzKRMtcxtLPXw==")) && ai3Var.l && (num = ai3Var.c) != null && num.intValue() == 0) {
                            break;
                        }
                    }
                    ai3 ai3Var2 = (ai3) next;
                    j2 = ai3Var2 != null ? ai3Var2.a : 0L;
                } else {
                    j2 = ai3VarK.a;
                }
                long j3 = j2;
                long j4 = tm1Var.g;
                pj3 pj3Var3 = this.I;
                String str = (pj3Var3 == null || (listE = pj3Var3.e()) == null) ? null : (String) listE.get(0);
                if (str == null) {
                    str = "";
                }
                if (gi3.A(j3, j4, str, "")) {
                    return;
                }
            }
            if (vh3Var != vh3.g && (pj3Var = this.I) != null) {
                int length = pj3Var.g.length;
                for (int i2 = 0; i2 < length; i2++) {
                    ai3 ai3VarK2 = pj3Var.k(i2);
                    if (ai3VarK2 != null) {
                        pj3Var.g[i2].b = ai3VarK2.a;
                    }
                    pj3.b(pj3Var.g[i2]);
                }
                hj3[] hj3VarArr = pj3Var.g;
                ArrayList arrayList = new ArrayList(hj3VarArr.length);
                for (hj3 hj3Var : hj3VarArr) {
                    arrayList.add(Long.valueOf(hj3Var.b));
                }
                pj3Var.l(arrayList);
            }
            if (vh3Var != vh3.g) {
                d dVar = this.T;
                int i3 = vh3Var == vh3.p ? 1 : 0;
                synchronized (dVar) {
                    try {
                        dVar.f(m54.a("s1:fs/6rY0c2meZbJ/eO4zPFRjsqitYdhULLo7/M3HyY6cmP1gpZE0T5A=="), new JSONObject().put(m54.a("s1:9APGw38pIRmFFr7xugW++AnUv7+waqJhCCY5sZBpeo/SFsH7"), i3));
                    } catch (JSONException e) {
                        throw new IllegalStateException(e);
                    }
                }
            }
            JSONObject jSONObject = new JSONObject();
            jSONObject.put(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn"), ((nh3) tm1Var.o).name());
            for (Object obj2 : N()) {
                ai3 ai3Var3 = (ai3) obj2;
                if (ai3Var3.a == tm1Var.g) {
                    if (gy1.n(ai3Var3.f, m54.a(vh3Var == vh3.g ? "s1:9vE/KjVA3hwcr9gEI5UbEtl78jVONDicVQw24DjmDd7xdjBJvLI=" : "s1:MI/KfCDPxwtR0UsTnxJV8ADHvsD+y/XljwzKRMtcxtLPXw=="))) {
                        obj = obj2;
                        break;
                    }
                }
            }
            ai3 ai3Var4 = (ai3) obj;
            if (ai3Var4 != null) {
                jSONObject.put("id", ai3Var4.a);
                jSONObject.put(m54.a("s1:cJDIk8tnu2VoRaX19OPJJYsFSsScNUfAk+9kC/k7AJxEYmM2qmY="), ai3Var4.g);
                jSONObject.put(m54.a("s1:zo0nI4eOpXX2DXIpIdchM810wjvWSVvg+WypFv49soygKCnsxW/c8WcuFLk="), ai3Var4.h);
                jSONObject.put(m54.a("s1:TMssgSxGXZmff7q8MS9XkmIJdd3ME0tGwmidPh1jFsLVUWvTp4A="), ai3Var4.i);
                String str2 = (String) this.a0.get(Long.valueOf(ai3Var4.a));
                if (str2 != null) {
                    jSONObject.put(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"), str2);
                }
            }
            qi qiVar = qi.a;
            String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
            nh3 nh3Var = (nh3) tm1Var.o;
            long j5 = tm1Var.g;
            StringBuilder sb = new StringBuilder();
            y82.s("s1:qL7pXIiuuYjt7hJxV9lreqVvJ+HWPwRUTWMmYj/WiRMDImjv3HnYugWzay7ThBVy", sb, j, "s1:wQlnYCT8N+i9/E11NDSEVBQd7xoFYtWiRCyovNSX5z3KLrnGF2EOVg==");
            sb.append(vh3Var);
            sb.append(m54.a("s1:u5aEolz4PAYZ75/s0/kFEoShmE9XcVg2q/jVQwv6DQYT5rtmqOZr/w=="));
            sb.append(nh3Var);
            sb.append(m54.a("s1:9jtU1kNgnjnQqHOQHXvjE3AyzKR+IOTXALBVpmaxlSuLDU6V"));
            sb.append(j5);
            qi.a(strA, sb.toString());
            final String str3 = this.Z;
            final int i4 = this.U0.get();
            final String str4 = (String) this.W.get(jSONObject.optString(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO")));
            m(new jo3(i, (Object) this, (Object) vh3Var, (Object) jSONObject), new ko3(i, vh3Var, this), new mf1() { // from class: lo3
                /* JADX WARN: Code duplicated, block: B:28:0x0075  */
                @Override // defpackage.mf1
                public final Object j(Object obj3) {
                    boolean z;
                    long j6;
                    boolean z2;
                    Integer num2;
                    boolean z3;
                    String str5;
                    sh3 sh3Var;
                    ArrayList arrayList2;
                    Integer num3;
                    tm1 tm1Var2 = tm1Var;
                    long j7 = tm1Var2.g;
                    Player player = (Player) obj3;
                    player.getClass();
                    vh3 vh3Var2 = vh3.o;
                    vh3 vh3Var3 = vh3Var;
                    if (vh3Var3 == vh3Var2 && (sh3Var = (sh3) player.t().get()) != null && (arrayList2 = sh3Var.b) != null && !arrayList2.isEmpty()) {
                        int size = arrayList2.size();
                        int i5 = 0;
                        while (true) {
                            if (i5 >= size) {
                                z = false;
                                break;
                            }
                            Object obj4 = arrayList2.get(i5);
                            i5++;
                            ai3 ai3Var5 = (ai3) obj4;
                            if (ai3Var5.l && gy1.n(ai3Var5.f, m54.a("s1:MI/KfCDPxwtR0UsTnxJV8ADHvsD+y/XljwzKRMtcxtLPXw==")) && (num3 = ai3Var5.c) != null && num3.intValue() == 1 && ai3Var5.a == j7) {
                                z = true;
                                break;
                            }
                        }
                    } else {
                        z = false;
                        break;
                    }
                    long j8 = j;
                    CompletableFuture completableFutureD = player.D(tm1Var2, vh3Var3, j8);
                    nh3 nh3Var2 = (nh3) tm1Var2.o;
                    Integer num4 = (Integer) completableFutureD.get();
                    if (num4 != null && num4.intValue() == 0) {
                        if (nh3Var2 != nh3.o) {
                            sh3 sh3Var2 = (sh3) player.t().get();
                            if (sh3Var2 != null) {
                                j6 = j8;
                                if (sh3Var2.a == j6) {
                                    ArrayList arrayList3 = sh3Var2.b;
                                    m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                                    arrayList3.getClass();
                                    if (gi3.B(arrayList3, vh3Var3, nh3Var2 == nh3.g ? -1L : j7, z).getBoolean(m54.a("s1:JG00ENWTf6nMmP5/OEFjXo82Q4hMwj9YajI10HbaT9BispSdrZNYITpghIQ="))) {
                                    }
                                }
                            } else {
                                j6 = j8;
                            }
                            z2 = false;
                        } else {
                            j6 = j8;
                        }
                        z2 = true;
                    } else {
                        j6 = j8;
                        z2 = false;
                    }
                    qi qiVar2 = qi.a;
                    qi.a(m54.a("s1:gwLuLUmwmaMxvWwQJvCFsaRW/5R70HpWdo0c/jdHWbkuDxfUqXl4wHhtdGgy4OUdSgvHd6nxxZk="), m54.a("s1:HaMwzb4rE4LEvDOHOnmq3P9oi0oxcthPN2y9++08WR8f+oGOlbcurJis20LcuR/7gpa1MkApZZRP+f73SSuQcgG0FxVLHXhth25M2to585vvdRXCugDf2i38") + vh3Var3 + m54.a("s1:nsuLaxvfp0hioEg5D/c/df+h2qb7HptaP+zCbGpQnhI3FqWs49MmmIueQNY=") + num4 + m54.a("s1:aSVYRCw6wVzhWz+KWxnFkb8XheCVr7s1uhB8q3b9v/3e57FmVAcoAVquh3XoH4vB") + z2);
                    up3 up3Var = this;
                    if (z2) {
                        sh3 sh3Var3 = (sh3) player.t().get();
                        String str6 = str4;
                        if (sh3Var3 != null) {
                            ArrayList arrayList4 = sh3Var3.b;
                            m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg==");
                            arrayList4.getClass();
                            z3 = z;
                            str5 = str6;
                            up3Var.P(gi3.S(arrayList4, tz0.g), gi3.R(vh3Var3), nh3Var2 == nh3.g ? "-1" : String.valueOf(j7), str5, z3);
                        } else {
                            z3 = z;
                            str5 = str6;
                        }
                        num2 = num4;
                        up3Var.Y(player, vh3Var3, tm1Var2, j6, str3, i4, str5, z3);
                    } else {
                        num2 = num4;
                    }
                    up3Var.v1 = 0;
                    return Integer.valueOf(z2 ? num2.intValue() : -1);
                }
            });
        }
    }

    public final void e(boolean z) {
        f0(null);
        f();
        g0(new zp3(0L, (String) null, 0.0d, 0.0d, 0.0d, z, false, 0, 0, 0L, (String) null, (Boolean) null, (String) null, (String) null, false, 0L, false, false, false, false, false, false, 0L, 0L, 0L, (String) null, false, 0, (String) null, 1073741759));
        this.q0.setValue(Double.valueOf(0.0d));
        xb3 xb3Var = this.f0;
        sz0 sz0Var = sz0.g;
        xb3Var.setValue(sz0Var);
        this.g0.setValue(sz0Var);
        this.k0.setValue(null);
        List list = Collections.EMPTY_LIST;
        this.j0.setValue(new zf3(false, false, list, list));
        this.o0.setValue(sz0Var);
        this.p0.setValue("");
        this.n0.setValue(null);
        this.i1.setValue(m54.a("s1:YijQ907msR/0eaJD6HxhroI97jqrRs2MVWqVMy0gyHj5kA=="));
    }

    public final void e0(final double d, final long j, final mf1 mf1Var) {
        m54.a("s1:OfGsOmclMYlfQqOJclrXTxTk4FNqYm6asdyju8ueYZdzdIfUjEmZPy3rnAjHwGlF");
        if (!K().k() || j != K().a || Math.abs(d) > Double.MAX_VALUE || 0.5d > d || d > 8.0d) {
            mf1Var.j(Boolean.FALSE);
            return;
        }
        qi qiVar = qi.a;
        String strA = m54.a("s1:AodeLleWIm9585WmCvU47aiQ87T65WydYYN+lIMLNJwmqNZ94FofFJk7UuI=");
        StringBuilder sb = new StringBuilder();
        y82.s("s1:aLRdRMNtTy04lL92w79Ep8w2mivToxqJFATO/tf+l1Bhfim3JO3GjwbyB+hJTu6D1sgbEw==", sb, j, "s1:I1op+yDp+Pemw872Rx1V60gY9wd4duU0F2LYtpsZ2Fh7nejM5N0sxw==");
        sb.append(d);
        qi.a(strA, sb.toString());
        m(new kf1() { // from class: np3
            @Override // defpackage.kf1
            public final Object a() {
                up3 up3Var = this;
                if (j == up3Var.K().a) {
                    xb3 xb3Var = up3Var.s0;
                    double d2 = d;
                    xb3Var.setValue(Double.valueOf(d2));
                    up3Var.F.j(Double.valueOf(d2));
                    mf1Var.j(Boolean.TRUE);
                }
                return n95.a;
            }
        }, new kf1() { // from class: op3
            @Override // defpackage.kf1
            public final Object a() {
                if (j == this.K().a) {
                    mf1Var.j(Boolean.FALSE);
                }
                return n95.a;
            }
        }, new ro3(d, 2));
    }

    public final void f() {
        if (G() == null && ((Bitmap) this.G0.getValue()) == null) {
            return;
        }
        this.I0++;
        this.H0.setValue(null);
        this.G0.setValue(null);
        qi qiVar = qi.a;
        qi.a(m54.a("s1:hG6dzE1TatVbxTmCddXSZ8/rHV8RWy1vJ1/RjbjObmPy3DDZUSJz3NTJiTk="), m54.a("s1:1eJaMUMS9SJm+wqE+Puf9Bb8kRxfUw4h6pq6J5sYY63OVcVD/axB+Nec8riO+v5WNXFvZSXlJ3yrsSYT") + this.O0 + m54.a("s1:fYtd2OpbHQO/1p31LgK9sgim0pepJ8YmD8yHuKqD2R5jywBeBlXo7dME9EQqn3p2aAAj4gxicsnJD7iTw4k=") + this.P0 + m54.a("s1:QWQRLszF46bwgIACVmxyLBeIphLOBGj1MtmCAVXoOxAk1MgH9YD9uiyDPpLGMX1bZc0=") + this.Q0 + m54.a("s1:Ggar9+zQh2+97PyAFYnks3QlFGS38J7K43V1D77snxTst1e1CUxCxDxzWCp3lls6IRg=") + (SystemClock.elapsedRealtime() - this.N0));
        if (this.V0) {
            return;
        }
        this.T0.execute(new fh(this, this.U0.get(), 1));
    }

    public final void f0(u94 u94Var) {
        this.C0.setValue(u94Var);
    }

    public final CompletableFuture g() {
        if (this.c1.compareAndSet(false, true)) {
            if (gy1.n(Looper.myLooper(), Looper.getMainLooper())) {
                h();
            } else {
                this.R0.post(new uo3(this, 3));
            }
        }
        CompletableFuture completableFutureThenApply = this.Y0.thenApply((Function) new ap3(new xp2(16)));
        m54.a("s1:CJhNE87w9yxM8Dpjr2E1GnJHXFvgk9eNTf98b96pjTBTvSntg68adRAU+/c7Nr/C2T05nBjAMME=");
        completableFutureThenApply.getClass();
        return completableFutureThenApply;
    }

    public final void g0(zp3 zp3Var) {
        this.e0.setValue(zp3Var);
    }

    public final void h() {
        uo3 uo3Var = new uo3(this, 4);
        ExecutorService executorService = this.T0;
        executorService.execute(uo3Var);
        U();
        final int i = 1;
        final byte b = 0;
        this.Y0.whenComplete((BiConsumer) new bp3(i, new as2(this, i, b)));
        pj3 pj3Var = this.I;
        if (pj3Var != null) {
            for (hj3 hj3Var : pj3Var.g) {
                pj3.b(hj3Var);
            }
            z4.C(pj3Var.h, null);
        }
        uo3 uo3Var2 = this.O;
        Handler handler = this.R0;
        handler.removeCallbacks(uo3Var2);
        this.M.close();
        handler.removeCallbacks(this.N);
        try {
            this.l.j(M());
        } catch (Exception unused) {
            qi qiVar = qi.a;
            qi.e(m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc="), m54.a("s1:rDF1edLViHkx8rxTrybJT5e/q9Bz9mUsyEODP2wnKHOrpGbn6VlwQBqsMkc="));
        }
        final CompletableFuture completableFutureB = um1.B(m54.a("s1:jj5B6n5UvzOstTl+1jLv0XUmPh9ERpTFubjkNsR9+Tg79zNMPnbcX7Q8W2w3o/NoxQOjVQfAEyI="), false, new yj3(this, 8));
        final CompletableFuture<Void> completableFutureAllOf = CompletableFuture.allOf(um1.B(m54.a("s1:0kwfVcFfF2JsaXhrYgVjx/d31mC41F24FchKsD9ecj9CGcUyXRU="), false, new yj3(this, 9)), completableFutureB);
        if (!this.V0) {
            this.D0.incrementAndGet();
            g0(zp3.a(K(), null, y(), 0.0d, sz0.g, false, 0, null, 1073741787));
            f0(null);
            f();
            this.T.e();
            this.U0.incrementAndGet();
            qp3 qp3Var = this.w1;
            if (qp3Var != null) {
                r(qp3Var, false, null);
            }
            CompletableFuture completableFuture = this.y1;
            if (completableFuture != null) {
                completableFuture.completeExceptionally(new IllegalStateException(m54.a("s1:HezXQId4yUxDHM6mBjopBeBWF777pNJpKqg7r2JT12exlab6AdNYTk4TaGAe4jQ//hmm+rXBVlLh8Q8psTWL8F/U")));
            }
            this.y1 = null;
            handler.removeCallbacks(this.x1);
            executorService.execute(new Runnable() { // from class: hp3
                @Override // java.lang.Runnable
                public final void run() {
                    int i2 = i;
                    up3 up3Var = this;
                    CompletableFuture completableFuture2 = completableFutureB;
                    switch (i2) {
                        case 0:
                            completableFuture2.whenComplete((BiConsumer) new bp3(4, new as2(up3Var, 3, (byte) 0)));
                            return;
                        default:
                            try {
                                return;
                            } finally {
                                up3Var.X();
                            }
                    }
                }
            });
        }
        this.V0 = true;
        fn3 fn3Var = this.z;
        if (fn3Var != null) {
            z4.C(fn3Var.d, null);
        }
        this.X0.completeExceptionally(new IllegalStateException(m54.a("s1:FlqrepFUTqjQhy328d1PdMA+sLXtHl6tZM3eWDHZ5iogzEp7b/CP9ds47ewGxgX/RZZ68nvbHfxI9cPVmIgtdw==")));
        executorService.execute(new Runnable() { // from class: hp3
            @Override // java.lang.Runnable
            public final void run() {
                int i2 = b;
                up3 up3Var = this;
                CompletableFuture completableFuture2 = completableFutureAllOf;
                switch (i2) {
                    case 0:
                        completableFuture2.whenComplete((BiConsumer) new bp3(4, new as2(up3Var, 3, (byte) 0)));
                        return;
                    default:
                        try {
                            return;
                        } finally {
                            up3Var.X();
                        }
                }
            }
        });
        ((ExecutorService) this.S0.p).shutdown();
        this.i.a();
    }

    public final void h0(String str) {
        String strV;
        m54.a("s1:j4Ff+VRxfJIUr+IVnVtJKw5+nxg1RKjC4MlmozwKF6psjTjl");
        str.getClass();
        JSONObject jSONObjectB = ei3.b(0, str, "", false);
        zp3 zp3VarK = K();
        String strOptString = jSONObjectB.optString(m54.a("s1:j4Ff+VRxfJIUr+IVnVtJKw5+nxg1RKjC4MlmozwKF6psjTjl"), m54.a("s1:vodTiYQgJay5T4BBrzJtQcEf7COjsh4XxW6xj2kj45oqPErYMr/hE7W9NXXgHml/duOx+oN+yELSoQ=="));
        m54.a("s1:bS90dHkkwXVlWWg2Z9x8x5xit2TOJbreYp7Y3lChPEeDKKR4wbS7tmfK2zc4ViJiLzuochEfCBY=");
        strOptString.getClass();
        g0(zp3.a(zp3VarK, null, 0.0d, 0.0d, null, false, 0, strOptString, 536870911));
        if (jSONObjectB.optInt(m54.a("s1:+fUCV81qE/MAjAvubwqShT4dEIPctIEhargJ7jSi1Jez8LZoraIAa8/VB1wZzl18")) > 0) {
            strV = uc0.n(jSONObjectB.getInt(m54.a("s1:+fUCV81qE/MAjAvubwqShT4dEIPctIEhargJ7jSi1Jez8LZoraIAa8/VB1wZzl18")), "s1:isVaWD4cz/pbNKBmulUsdYg5osOOa/QE4ID6dDCnnaSWVeRukho=", new StringBuilder());
        } else {
            String strOptString2 = jSONObjectB.optString(m54.a("s1:61jkhyDvHYqkZQUe+ZrQyog03RiohgPlJ0U/BuBhAKGF9A=="), m54.a("s1:43KVA6WwTRvJfLM6uBlUkUIBpeK+tT5WETMnn7QRllvFzRw6K05MeDkTjAlUq0arE76vLVdLEtf/G6fMSFWCqw=="));
            m54.a("s1:bS90dHkkwXVlWWg2Z9x8x5xit2TOJbreYp7Y3lChPEeDKKR4wbS7tmfK2zc4ViJiLzuochEfCBY=");
            strOptString2.getClass();
            strV = u9.V(strOptString2, new Object[0]);
        }
        i0(strV, true);
    }

    public final void i(zp3 zp3Var) {
        if (this.c0 && zp3Var.c()) {
            this.c0 = false;
            qi qiVar = qi.a;
            String strA = m54.a("s1:AodeLleWIm9585WmCvU47aiQ87T65WydYYN+lIMLNJwmqNZ94FofFJk7UuI=");
            long j = zp3Var.a;
            long jElapsedRealtime = SystemClock.elapsedRealtime() - this.d0;
            StringBuilder sb = new StringBuilder();
            y82.s("s1:JCvf3MHb1KzTSrp/Uvc4Irs/dEDWMp2cXAM8XdDWn95lDPkaXMZ5sxlfOKoQSlqpUsTu1I8c+fMrJLyuZ1czpHQsSpVThi+dZeEVQTShMgtH1rlgCeFGNYthP84bgR9Y", sb, j, "s1:Zl+idTEaIG68xy6mqoff0bQ2wtTYZ6Iz+wmARScBLvzT1rFMK4vXNVJRVCEA8WP+GNpORw==");
            sb.append(jElapsedRealtime);
            qi.a(strA, sb.toString());
        }
    }

    public final void i0(String str, boolean z) {
        m54.a("s1:yQWhDK92HykSkL4nv6kWZ/F+wThSpruswGUywUo2+0BBPXCYUeojzDBG");
        str.getClass();
        if (this.V0) {
            if (!z) {
                return;
            } else {
                p();
            }
        }
        this.u0.setValue(Boolean.valueOf(z));
        this.t0.setValue(str);
        vb3 vb3Var = this.v0;
        vb3Var.h(vb3Var.g() + 1);
    }

    public final void j(int i, kf1 kf1Var) {
        this.R0.post(new l80(this, i, kf1Var, 5));
    }

    public final void j0(double d) {
        if (!K().k() || Math.abs(d) > Double.MAX_VALUE || 0.25d > d || d > 8.0d) {
            return;
        }
        qi qiVar = qi.a;
        String strA = m54.a("s1:AodeLleWIm9585WmCvU47aiQ87T65WydYYN+lIMLNJwmqNZ94FofFJk7UuI=");
        long j = K().a;
        StringBuilder sb = new StringBuilder();
        y82.s("s1:0yVlIH8v6jHwz2af/JmoUDPaVh64GkKgQ7im0KBS3CkqHCXfJ103sXC0tgWka37z5OdCwg==", sb, j, "s1:I1op+yDp+Pemw872Rx1V60gY9wd4duU0F2LYtpsZ2Fh7nejM5N0sxw==");
        sb.append(d);
        qi.a(strA, sb.toString());
        int i = 0;
        n(this, new qo3(this, d, i), null, new ro3(d, i), 2);
    }

    public final void k() {
        qi qiVar = qi.a;
        qi.a(m54.a("s1:AodeLleWIm9585WmCvU47aiQ87T65WydYYN+lIMLNJwmqNZ94FofFJk7UuI="), m54.a("s1:5iUG2ZQEPozih+oJxZr3NjfJldGmePxn4vSgoAhGTGESFEqrkDTxPVcjtc0GmqhajkZ0Ym6AIsj4FNjLGgollsNMzgTlLkWkNjBPYyVAIXEWhCb5F+ZxfQv2") + this.p1);
        this.e1 = null;
        this.k1 = null;
        this.l1 = null;
        S();
        if (this.o1 && this.b1) {
            k0();
        }
    }

    public final void k0() {
        Surface surface;
        Surface surface2;
        V();
        if (this.V0 || this.W0 || this.c1.get() || (surface = this.e1) == null || !surface.isValid() || (surface2 = this.l1) == null || !surface2.isValid()) {
            return;
        }
        if (!this.p1) {
            final eg3 eg3Var = this.z1;
            if (eg3Var == null) {
                l0(null, y(), this.q1, null);
                return;
            }
            int i = this.U0.get();
            final double dY = y() * 1000.0d;
            final boolean z = this.q1;
            eg3Var.e(new Supplier() { // from class: cg3
                @Override // java.util.function.Supplier
                public final Object get() {
                    return eg3Var.b(dY, z);
                }
            }).whenComplete((BiConsumer) new rw(29, new n42(this, i, 4)));
            return;
        }
        if (this.b1) {
            S();
            return;
        }
        this.a1 = false;
        final Surface surface3 = this.e1;
        if (surface3 == null) {
            kb2.j(m54.a("s1:vt4o3Zjf9BnrLNsOBVeZwcLDIKoTkuAgmTv4biDhLxsfXmnwF7o1tfN7Dh2jydGgavX08lB7dwdsyBXJtC591zT8BcYB+Da7jGU8Qw=="));
            return;
        }
        final Surface surface4 = this.l1;
        if (surface4 == null) {
            kb2.j(m54.a("s1:vt4o3Zjf9BnrLNsOBVeZwcLDIKoTkuAgmTv4biDhLxsfXmnwF7o1tfN7Dh2jydGgavX08lB7dwdsyBXJtC591zT8BcYB+Da7jGU8Qw=="));
            return;
        }
        final int i2 = this.m1;
        final int i3 = this.n1;
        final boolean z2 = this.q1;
        n(this, null, null, new mf1() { // from class: xo3
            @Override // defpackage.mf1
            public final Object j(Object obj) {
                Player player = (Player) obj;
                m54.a("s1:EObpmTi+JSeKC1u6W5cfvocjuFDeeXLOmr0lxFKT//BxGYS8Uipb6k9t");
                player.getClass();
                Integer num = (Integer) player.x(surface3, surface4).get();
                if (num != null && num.intValue() == 0) {
                    player.B(i2, i3);
                    player.y(z2);
                }
                if (num == null || num.intValue() != 0) {
                    up3 up3Var = this;
                    up3Var.R0.post(new uo3(up3Var, 5));
                }
                num.getClass();
                return num;
            }
        }, 3);
    }

    public final void l() {
        Double d = this.r1;
        if (d != null) {
            double dDoubleValue = d.doubleValue();
            this.r1 = null;
            j0(dDoubleValue);
            this.s0.setValue(d);
        }
    }

    public final void l0(JSONObject jSONObject, final double d, boolean z, final CompletableFuture completableFuture) {
        final boolean z2;
        boolean zN;
        boolean zN2;
        String strOptString;
        String strOptString2;
        JSONObject jSONObjectOptJSONObject;
        JSONObject jSONObjectOptJSONObject2;
        JSONObject jSONObjectOptJSONObject3;
        if (this.V0 || this.W0 || this.c1.get()) {
            if (completableFuture != null) {
                completableFuture.completeExceptionally(new IllegalStateException(m54.a("s1:FlqrepFUTqjQhy328d1PdMA+sLXtHl6tZM3eWDHZ5iogzEp7b/CP9ds47ewGxgX/RZZ68nvbHfxI9cPVmIgtdw==")));
                return;
            }
            return;
        }
        V();
        final Surface surface = this.e1;
        final Surface surface2 = this.l1;
        if (surface == null || surface2 == null || !surface.isValid() || !surface2.isValid()) {
            if (completableFuture != null) {
                completableFuture.completeExceptionally(new IllegalStateException(m54.a("s1:Rspnvi2eugR/cR24Gu65URY2OqTLEqLgpNHPj9vSUx7T/nkS4F7Bdy33wVqFFaoKLwmVLxdHxETWpXMWxaFR4zF0oDI=")));
                return;
            }
            return;
        }
        qp3 qp3Var = this.w1;
        if (qp3Var != null) {
            r(qp3Var, false, null);
        }
        this.p1 = true;
        this.y1 = completableFuture;
        if (this.u != null || jSONObject == null || gy1.n(jSONObject.optString(m54.a("s1:Q95/7cZorYq8vCixrcbxHT4ot4ZvtMMP7vjL07lEuKvXdA==")), this.a) || K().a == 0 || N().isEmpty()) {
            z2 = true;
        } else {
            d dVar = new d("", "");
            z2 = true;
            dVar.f(m54.a("s1:dI8VM1k+Tt7EpvdrLMzFG5IK9vbp3g6lXNpm/cB2qOEfcrFvc8s+H4ScuJelIQ=="), new JSONObject().put(m54.a("s1:moJWpINXfzQZjGiePwZQZ/2NF3AerPIgSzUvVNLReobnfhvWzkvnZQ=="), K().a).put(m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg=="), gi3.E(N(), this.a0)));
            JSONObject jSONObject2 = new JSONObject(dVar.b.getJSONObject(m54.a("s1:Bh/y7YBJpf5LLdXwB5soWfQbDO+NoZbF4SxPF6k5JL9VTzKlU7nWiaQg")).toString());
            if (this.R == null && (jSONObjectOptJSONObject3 = jSONObject2.optJSONObject(m54.a("s1:9vE/KjVA3hwcr9gEI5UbEtl78jVONDicVQw24DjmDd7xdjBJvLI="))) != null) {
                this.R = gi3.L(jSONObjectOptJSONObject3);
            }
            if (this.P == null && (jSONObjectOptJSONObject2 = jSONObject2.optJSONObject(m54.a("s1:Y+IrCff8CTSYNqNe0Dgq4gIvM4GPVAUeq9ZXx31lzJOcdXpx8I7Y938kAVo="))) != null) {
                this.P = gi3.L(jSONObjectOptJSONObject2);
            }
            if (this.Q == null && (jSONObjectOptJSONObject = jSONObject2.optJSONObject(m54.a("s1:/bcqGsQMlUwQJmpCAectwP6CtNea/85YbqojUdDH0aEOlUnlT6+mBlNEvrnCFA=="))) != null) {
                this.Q = gi3.L(jSONObjectOptJSONObject);
            }
        }
        if (jSONObject != null) {
            this.Z = y82.j("s1:VZVGcaIe1ZxoSjO+F8Fh3Jk09o0FjVN52Gs5Htiel4RWzGi9xxB5vg==", "s1:bS90dHkkwXVlWWg2Z9x8x5xit2TOJbreYp7Y3lChPEeDKKR4wbS7tmfK2zc4ViJiLzuochEfCBY=", jSONObject);
            JSONObject jSONObject3 = this.P;
            if (jSONObject3 == null || (strOptString2 = jSONObject3.optString(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"))) == null) {
                zN = false;
            } else {
                String str = (String) this.V.get(strOptString2);
                zN = gy1.n(str != null ? Boolean.valueOf(!str.equals(this.Z)) : null, Boolean.TRUE);
            }
            if (zN) {
                this.P = null;
            }
            JSONObject jSONObject4 = this.Q;
            if (jSONObject4 == null || (strOptString = jSONObject4.optString(m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO"))) == null) {
                zN2 = false;
            } else {
                String str2 = (String) this.V.get(strOptString);
                zN2 = gy1.n(str2 != null ? Boolean.valueOf(!str2.equals(this.Z)) : null, Boolean.TRUE);
            }
            if (zN2) {
                this.Q = null;
            }
            this.a = uc0.o("s1:Q95/7cZorYq8vCixrcbxHT4ot4ZvtMMP7vjL07lEuKvXdA==", "s1:SCLwIEf5imwB/Y1PMtx/o7X/isEo3k3f60De8HzYMbsftu/24UsspjYTjlZqwYeZkv8kVr66XpY=", jSONObject);
            JSONObject jSONObjectOptJSONObject4 = jSONObject.optJSONObject(m54.a("s1:a6aG1OmGxysoJS/oLHc1FStaZ2Ibzv/zPk30d6nlTn341AaAwmIK/3yu"));
            if (jSONObjectOptJSONObject4 == null) {
                jSONObjectOptJSONObject4 = new JSONObject();
            }
            Iterator<String> itKeys = jSONObjectOptJSONObject4.keys();
            m54.a("s1:KEIV5HJJs4nkN8qY+AOytsCyUlwamYKbpJY6sS6tvJ8AP3pT+LZkA6wG65JC4Q==");
            itKeys.getClass();
            je4 je4VarU = le4.U(itKeys);
            LinkedHashMap linkedHashMap = new LinkedHashMap();
            for (Object obj : (zb0) je4VarU) {
                linkedHashMap.put(obj, jSONObjectOptJSONObject4.getString((String) obj));
            }
            this.m = linkedHashMap;
        }
        final String str3 = this.Z;
        tz0 tz0Var = tz0.g;
        this.X.setValue(tz0Var);
        this.Y = tz0Var;
        this.c0 = false;
        final String str4 = this.a;
        final Map map = this.m;
        final Display display = this.k1;
        final int iIncrementAndGet = this.U0.incrementAndGet();
        this.R0.removeCallbacks(this.x1);
        this.q1 = z;
        if (!z) {
            this.q1 = !((Boolean) this.h.a()).booleanValue();
        }
        f();
        final boolean z3 = this.q1;
        final double dH = H();
        final double dT = t();
        final double dIntValue = ((double) ((Number) this.B0.getValue()).intValue()) / 100.0d;
        final int i = this.m1;
        final int i2 = this.n1;
        final String str5 = this.A1;
        int iIntValue = this.k ? 0 : ((Number) this.g1.getValue()).intValue();
        int iJ = (!((Boolean) this.j1.getValue()).booleanValue() || this.k) ? 0 : J();
        if (!O() || this.k) {
            z2 = false;
        }
        final int i3 = iIntValue;
        final zv4 zv4VarM = M();
        final JSONObject jSONObject5 = this.P;
        final JSONObject jSONObject6 = this.Q;
        final JSONObject jSONObject7 = this.R;
        final JSONObject jSONObject8 = new JSONObject(this.S.toString());
        LinkedHashMap linkedHashMap2 = this.U;
        final LinkedHashMap linkedHashMap3 = new LinkedHashMap();
        Iterator it = linkedHashMap2.entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry entry = (Map.Entry) it.next();
            Iterator it2 = it;
            String str6 = (String) this.V.get((String) entry.getKey());
            if (str6 != null ? str6.equals(this.Z) : true) {
                linkedHashMap3.put(entry.getKey(), entry.getValue());
            }
            it = it2;
        }
        final String str7 = this.l0;
        zp3 zp3VarK = K();
        sz0 sz0Var = sz0.g;
        g0(zp3.a(zp3VarK, m54.a("s1:4wdb9LL5ehF4WmToSWWNJ1bjKujVYkhBujd9fmlaN6o46OvYPfk24XIv"), 0.0d, 0.0d, sz0Var, z3, 0, null, 805306268));
        this.q0.setValue(Double.valueOf(0.0d));
        this.f0.setValue(sz0Var);
        this.k0.setValue(null);
        List list = Collections.EMPTY_LIST;
        this.j0.setValue(new zf3(false, false, list, list));
        this.o0.setValue(sz0Var);
        this.p0.setValue("");
        this.n0.setValue(null);
        final int i4 = iJ;
        this.T0.execute(new Runnable() { // from class: ip3
            @Override // java.lang.Runnable
            public final void run() {
                up3.m0(this.g, iIncrementAndGet, completableFuture, surface, surface2, str5, str4, display, i, i2, dH, dT, dIntValue, z2, i3, i4, str7, map, d, z3, linkedHashMap3, str3, jSONObject7, jSONObject5, jSONObject6, jSONObject8, zv4VarM);
            }
        });
    }

    public final void m(kf1 kf1Var, kf1 kf1Var2, mf1 mf1Var) {
        if (this.V0) {
            return;
        }
        this.T0.execute(new h92(this, this.U0.get(), mf1Var, kf1Var, kf1Var2));
    }

    public final ai3 o(String str) {
        Object obj;
        Object next;
        m54.a("s1:seiE/lOn7MziORMGxqrA0lVvWZJUCun1kVzQCPMoNAMbs0pO");
        Iterator it = this.a0.entrySet().iterator();
        do {
            obj = null;
            if (!it.hasNext()) {
                next = null;
                break;
            }
            next = it.next();
        } while (!gy1.n(((Map.Entry) next).getValue(), str));
        Map.Entry entry = (Map.Entry) next;
        if (entry == null) {
            return null;
        }
        long jLongValue = ((Number) entry.getKey()).longValue();
        for (Object obj2 : N()) {
            if (((ai3) obj2).a == jLongValue) {
                obj = obj2;
                break;
            }
        }
        return (ai3) obj;
    }

    public final void p() {
        g0(zp3.a(K(), m54.a("s1:lsK8qcjzW/Rb+kI8C/2Hd5GpzAKGeoUBwccUWUu1HdnTqdnvaeOuhg=="), 0.0d, 0.0d, sz0.g, true, -1, null, 805306269));
    }

    public final void q(int i, Throwable th) {
        qi qiVar = qi.a;
        qi.e(m54.a("s1:AodeLleWIm9585WmCvU47aiQ87T65WydYYN+lIMLNJwmqNZ94FofFJk7UuI="), y82.k("s1:QeVWBBhAKIZ3qGvkrBmiAfLdwC4vD3Eqymsh/MMqdH1sKufzU379c3m4ibU3HARS1Ixrax4jKtXuhAaC/WicTDO/l0EWYOoG/gIPGsDLxmiHbkvg", new StringBuilder(), qi.c(th), "s1:8GKSHBPrxv0fAjHCeaQgfoXdQ7H2cXrF47e5O3LgO4myslvT2o1udQ==", th.getClass().getSimpleName()));
        j(i, new ko3(this, th, 3));
    }

    public final void q0() {
        qi qiVar = qi.a;
        String strA = m54.a("s1:PEb8ZGpJM2WTJsEgM2uQVu2hCubZOFqk2OznyX2EN83vmrqPPeNJuS636Dc=");
        long j = K().a;
        boolean z = M().d;
        double d = M().a.c;
        double d2 = M().b.c;
        double d3 = M().a.d;
        double d4 = M().b.d;
        double d5 = M().f;
        double d6 = M().g;
        StringBuilder sb = new StringBuilder();
        y82.s("s1:3X9sdpVlrpLXv5MR/w4tpt1AVg4hpEfKh33iY8/u++F0rELSo23mZY1P1/pdKUAFXYpJyA==", sb, j, "s1:wrQn5d6P7wvQ7/oukko5jKiZAMdc8jqRICd5XHrkGPuMecyB+tUAk3auUo8=");
        sb.append(z);
        sb.append(m54.a("s1:FNylreXr5/qVnP3q62+AQ57jSuF+jXJYjPuvGHesPGP+teksmkuYn2l4dx157drjO+vMBcUjk5Y="));
        sb.append(d);
        sb.append(m54.a("s1:wu/U+il0qQkcFOPCZjZbCbh2xUkf6lqB79btBJAADkrGzkOGoUeyGokV4YuMBDsGZz8LtIcl7n/X/Kc+"));
        sb.append(d2);
        sb.append(m54.a("s1:kgiNiE3lrskvAE9969PXTbKndQlBFWKwf34LWG9KhOfpckMWGkZ4NOTeK9lyGVokbmGUd8Ra7+g="));
        sb.append(d3);
        sb.append(m54.a("s1:aYDpWKJfzonzu444jX3TxNGDf+XXQwK6gWmhko42scJto6mKmjVDBm1GhiyHvo2wx4Sj7ReJJaMY+N/k"));
        sb.append(d4);
        sb.append(m54.a("s1:EPGBItpHH/beKMTDjeV/MCqFK87QeqCuvMKlLiYDzMa8rCxzFGm4y5AhnKY="));
        sb.append(d5);
        sb.append(m54.a("s1:oihpZ6uYZOdCMijMF2iM9aqJs8FFlyCR3mZ9QYwl1RoTMxrOLOXzrX1ddspJvw=="));
        sb.append(d6);
        qi.a(strA, sb.toString());
    }

    public final void r(qp3 qp3Var, boolean z, Boolean bool) {
        if (this.w1 != qp3Var) {
            return;
        }
        this.w1 = null;
        if (z) {
            this.p = qp3Var.b;
        }
        qp3Var.c.i(Boolean.valueOf(z), bool);
    }

    public final String r0() {
        if (!K().k()) {
            return "";
        }
        eg3 eg3Var = this.z1;
        return (eg3Var == null || !eg3Var.c()) ? this.Z : "";
    }

    public final String s0() throws JSONException {
        JSONObject jSONObject = new JSONObject();
        jSONObject.put(m54.a("s1:uHsYvqIH2oKriEVprwwgrnlmiyQgHo4/5kqYKsV6TXxCJXzkUbGgqFlj"), this.P);
        jSONObject.put(m54.a("s1:/bcqGsQMlUwQJmpCAectwP6CtNea/85YbqojUdDH0aEOlUnlT6+mBlNEvrnCFA=="), this.Q);
        jSONObject.put(m54.a("s1:9vE/KjVA3hwcr9gEI5UbEtl78jVONDicVQw24DjmDd7xdjBJvLI="), this.R);
        jSONObject.put(m54.a("s1:jf3QNHREzzEDNtyR4ptoOPpa61u9EqzQc42+znkqIj3kVzMWPonQqMV+85JbWi9F"), this.S);
        jSONObject.put(m54.a("s1:Iuw9VTIaC00Y0Isve30BKmUwhcz4ZLFXRYnXdROlVqXhOyOeNWmWsjk340g="), new JSONObject(pj2.Z(this.U)));
        jSONObject.put(m54.a("s1:A7u239YFTKg9Rp+gprGyF+cBOuOWq+OPveIhbX4AcxSZ6VGVUFHQp91UpWIaSg2N1dCXoVcy"), new JSONObject(pj2.Z(this.V)));
        jSONObject.put(m54.a("s1:nz7QFqNC03Fwmbbg70vt2Hi4MycCBzOReFZd5yGI+U2lexA8lv526OMhN3gPAyF5nHhI8niyiqLQBQ=="), new JSONObject(pj2.Z(this.W)));
        String string = jSONObject.toString();
        m54.a("s1:ymwkKUy65sKHjBT7LR3qClvDZ1WRF5V7sXm6w1/qxYVcM25DQhnEJQ8ScORKMC3y0Hyk9eMw");
        string.getClass();
        return string;
    }

    public final double t() {
        return ((Number) this.A0.getValue()).doubleValue();
    }

    /* JADX WARN: Code duplicated, block: B:32:0x005c  */
    public final String t0(String str) {
        Object s34Var;
        List<Proxy> listSelect;
        Object next;
        SocketAddress socketAddressAddress;
        if (!this.o.w) {
            return null;
        }
        try {
            ProxySelector proxySelector = ProxySelector.getDefault();
            if (proxySelector == null || (listSelect = proxySelector.select(new URI(str))) == null) {
                s34Var = null;
            } else {
                Iterator<T> it = listSelect.iterator();
                do {
                    if (!it.hasNext()) {
                        next = null;
                        break;
                    }
                    next = it.next();
                } while (((Proxy) next).type() != Proxy.Type.HTTP);
                Proxy proxy = (Proxy) next;
                if (proxy == null || (socketAddressAddress = proxy.address()) == null) {
                    s34Var = null;
                } else {
                    InetSocketAddress inetSocketAddress = socketAddressAddress instanceof InetSocketAddress ? (InetSocketAddress) socketAddressAddress : null;
                    if (inetSocketAddress != null) {
                        String hostString = inetSocketAddress.getHostString();
                        hostString.getClass();
                        if (vs4.L(hostString)) {
                            hostString = null;
                        }
                        if (hostString == null) {
                            s34Var = null;
                        } else {
                            if (vs4.D(hostString, ':')) {
                                hostString = m54.a("s1:Ow+WS7hoDBWmsZxs9fdM6aRZapgZGsOtSWFH5GGN") + ((Object) hostString) + m54.a("s1:T4gfoHO5XDiSY3wNXy9xWgO4ChKSw25HXGA/RK8W");
                            }
                            s34Var = m54.a("s1:drfpsrgT3fY6iC7OV58spyzaPu7WQ7UlwO1D6Ec2tg+MJcpqx8r5fDeL") + ((Object) hostString) + m54.a("s1:fQEbEVsqNn72AG1Fx868qKzYFNrKAHiFNN/yXLRr") + inetSocketAddress.getPort();
                        }
                    } else {
                        s34Var = null;
                    }
                }
            }
        } catch (Throwable th) {
            s34Var = new s34(th);
        }
        return (String) (s34Var instanceof s34 ? null : s34Var);
    }

    public final int u() {
        return this.o.q;
    }

    public final void u0() {
        if (K().f()) {
            kf1 kf1Var = this.C1;
            if (kf1Var != null) {
                kf1Var.a();
                return;
            } else {
                a0();
                return;
            }
        }
        if (K().k()) {
            boolean z = K().g;
            boolean z2 = !z;
            if (z && !((Boolean) this.h.a()).booleanValue()) {
                i0(u9.V(m54.a("s1:+xRu3SiQ16DwkL3N3I2gBOmUWNq2JYeimRcJBVxiMbDcSbhFe3huAvZ0OWNjQwC2wrWBDNPbiVWN9imJ0QffbtyETZ+dvrElsAt3BHlrAfXCEzI/a9qD+dVVxXqr4PU5XA3XTC3n2Iwg5ihn5dw="), new Object[0]), false);
                return;
            }
            qi qiVar = qi.a;
            String strA = m54.a("s1:AodeLleWIm9585WmCvU47aiQ87T65WydYYN+lIMLNJwmqNZ94FofFJk7UuI=");
            long j = K().a;
            StringBuilder sb = new StringBuilder();
            y82.s("s1:gOFkDt4XwjMOv4GknfSeNxoEfILhswpI4FDoM1oSXvhzPkVcu+Zo0ziENcPftz1RG4qOQw==", sb, j, "s1:S0avfdjWEnKioKd/g6UtFvSIl76J/rG0ym53b6ukfm/J8MZMT2OxA0kYtV0=");
            sb.append(z2);
            qi.a(strA, sb.toString());
            this.q1 = z2;
            n(this, null, null, new bk(z2, 25), 3);
            if (z) {
                return;
            }
            this.i.a();
        }
    }

    public final boolean v() {
        JSONObject jSONObject;
        JSONArray jSONArrayOptJSONArray;
        Object obj;
        if (((Boolean) this.j1.getValue()).booleanValue() && K().k() && !this.k && (jSONObject = (JSONObject) this.n0.getValue()) != null && (jSONArrayOptJSONArray = jSONObject.optJSONArray(m54.a("s1:+9yjoNUIsA6IInvlWnCai0AxdQ81zl03fdq5oKEZZppzajCQ+Sq8Qg=="))) != null) {
            xw1 xw1VarC = jg5.C(0, jSONArrayOptJSONArray.length());
            ArrayList arrayList = new ArrayList();
            Iterator it = xw1VarC.iterator();
            while (true) {
                ww1 ww1Var = (ww1) it;
                if (!ww1Var.p) {
                    break;
                }
                JSONObject jSONObjectOptJSONObject = jSONArrayOptJSONArray.optJSONObject(ww1Var.nextInt());
                if (jSONObjectOptJSONObject != null) {
                    arrayList.add(jSONObjectOptJSONObject);
                }
            }
            ArrayList arrayList2 = new ArrayList();
            int size = arrayList.size();
            int i = 0;
            while (i < size) {
                Object obj2 = arrayList.get(i);
                i++;
                if (gy1.n(((JSONObject) obj2).optString(m54.a("s1:d3Vlw1OZkAszKOSSmVvLXcl1/3U6znBzqGs33V1vbf06vKrn")), m54.a("s1:ykA/aQnECxnqEbtKzLAqb3Qx+OcJaYRIwSzDB0dsGnn3byqD98U="))) {
                    arrayList2.add(obj2);
                }
            }
            int size2 = arrayList2.size();
            int i2 = 0;
            do {
                if (i2 >= size2) {
                    obj = null;
                    break;
                }
                obj = arrayList2.get(i2);
                i2++;
            } while (!((JSONObject) obj).optBoolean(m54.a("s1:YaiVdXnG5emFJPHl0gXYnDJT/Xt9FQ9b7bwqAGzoPcecs+v+oyPAPyd86us=")));
            JSONObject jSONObject2 = (JSONObject) obj;
            if ((jSONObject2 != null || (jSONObject2 = (JSONObject) b60.n1(arrayList2)) != null) && ct4.s(jSONObject2.optString(m54.a("s1:bhQtMKCejegTQK6B2js2sE++lahX5r7gLi4sUQ2fjxV+DJYjdobKbtRRmyZmaK8taUPniw==")), m54.a("s1:3SrlmTlJxm7Qd/54IVAIh649NdLM/PJHKCswQbOXXRqdeQ==")) && jSONObject2.optInt(m54.a("s1:Sny+2Xi70fcHgasG7Glfp2UUpZruCkBbzkK97o8P/7dEKmemqzkLYFcI7Gq/6A==")) == 0) {
                return true;
            }
        }
        return false;
    }

    public final String v0(String str) {
        JSONArray jSONArrayOptJSONArray;
        JSONObject jSONObjectOptJSONObject;
        if (this.u != null) {
            return str;
        }
        JSONObject jSONObject = this.y;
        String strOptString = (jSONObject == null || (jSONArrayOptJSONArray = jSONObject.optJSONArray(m54.a("s1:0kwfVcFfF2JsaXhrYgVjx/d31mC41F24FchKsD9ecj9CGcUyXRU="))) == null || (jSONObjectOptJSONObject = jSONArrayOptJSONArray.optJSONObject(0)) == null) ? null : jSONObjectOptJSONObject.optString("id");
        return strOptString == null ? "" : strOptString;
    }

    public final boolean w() {
        return K().k() && K().d > 0.0d && K().w && !this.k;
    }

    public final void w0(zv4 zv4Var) throws JSONException {
        pj3 pj3Var;
        m54.a("s1:cnlRbobsAHfu61HeRACg2LMiM7oVmfHrI4MQUo9YCXIbFR40NgU=");
        zv4Var.getClass();
        if (this.V0) {
            return;
        }
        JSONObject jSONObjectB = lt4.b(m54.a("s1:O5rkosCmahBadzZmK2ONI+CPsN7RyyAkZwHlnCgTcwgASe+T27nQVw=="), zv4Var.d(), false);
        m54.a("s1:gryZhHCz247p15B8Q3Sd5qrnpvZcQTK51N5AwVN+MpYiQwIioPldofUD+XcxDDJKB5SwtZxjtpc=");
        jSONObjectB.getClass();
        zv4 zv4VarM = tl3.m(jSONObjectB);
        if (zv4VarM.equals(M())) {
            return;
        }
        this.L.setValue(zv4VarM);
        if (!zv4VarM.c && (pj3Var = this.I) != null) {
            pj3Var.m(true);
            for (hj3 hj3Var : pj3Var.g) {
                pj3.b(hj3Var);
            }
            pj3Var.i(false);
        }
        Handler handler = this.R0;
        uo3 uo3Var = this.O;
        handler.removeCallbacks(uo3Var);
        handler.postDelayed(uo3Var, 400L);
        b0();
    }

    public final String x() throws JSONException {
        if (!gy1.n(this.J, this.a)) {
            String str = this.a;
            m54.a("s1:Q95/7cZorYq8vCixrcbxHT4ot4ZvtMMP7vjL07lEuKvXdA==");
            str.getClass();
            m54.a("s1:Cw10S/V9LEGjnVFQBm7tK+dycXmhO2h6PpoDXTZMREQ8F/A3S92upGn74/c=");
            String str2 = this.n;
            str2.getClass();
            String strQ = le2.Q(str, str2);
            this.K = strQ != null ? u9.V(strQ, new Object[0]) : null;
            this.J = this.a;
        }
        return this.K;
    }

    public final double y() {
        u94 u94Var = (u94) this.C0.getValue();
        return u94Var != null ? u94Var.c : K().c;
    }

    public final boolean z() {
        return this.k;
    }
}