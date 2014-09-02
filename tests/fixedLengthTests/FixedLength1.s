; ModuleID = 'FixedLength1.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind readonly uwtable
define i32 @foo(i32* nocapture %array) #0 {
vector.ph:
  %arrayidx9 = getelementptr inbounds i32* %array, i64 30
  %arrayidx6 = getelementptr inbounds i32* %array, i64 10
  %0 = load i32* %arrayidx9, align 4, !tbaa !0
  %1 = load i32* %arrayidx6, align 4, !tbaa !0
  %broadcast.splatinsert28 = insertelement <4 x i32> undef, i32 %1, i32 0
  %broadcast.splat29 = shufflevector <4 x i32> %broadcast.splatinsert28, <4 x i32> undef, <4 x i32> zeroinitializer
  %broadcast.splatinsert30 = insertelement <4 x i32> undef, i32 %0, i32 0
  %broadcast.splat31 = shufflevector <4 x i32> %broadcast.splatinsert30, <4 x i32> undef, <4 x i32> zeroinitializer
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %vec.phi = phi <4 x i32> [ zeroinitializer, %vector.ph ], [ %10, %vector.body ]
  %2 = getelementptr inbounds i32* %array, i64 %index
  %3 = bitcast i32* %2 to <4 x i32>*
  %wide.load = load <4 x i32>* %3, align 4
  %4 = add nsw <4 x i32> %wide.load, %vec.phi
  %5 = add i64 %index, 4
  %6 = getelementptr inbounds i32* %array, i64 %5
  %7 = bitcast i32* %6 to <4 x i32>*
  %wide.load27 = load <4 x i32>* %7, align 4
  %8 = add nsw <4 x i32> %4, %wide.load27
  %9 = add nsw <4 x i32> %8, %broadcast.splat29
  %10 = add nsw <4 x i32> %9, %broadcast.splat31
  %index.next = add i64 %index, 4
  %11 = icmp eq i64 %index.next, 40
  br i1 %11, label %for.end, label %vector.body

for.end:                                          ; preds = %vector.body
  %rdx.shuf = shufflevector <4 x i32> %10, <4 x i32> undef, <4 x i32> <i32 2, i32 3, i32 undef, i32 undef>
  %bin.rdx = add <4 x i32> %10, %rdx.shuf
  %rdx.shuf32 = shufflevector <4 x i32> %bin.rdx, <4 x i32> undef, <4 x i32> <i32 1, i32 undef, i32 undef, i32 undef>
  %bin.rdx33 = add <4 x i32> %bin.rdx, %rdx.shuf32
  %12 = extractelement <4 x i32> %bin.rdx33, i32 0
  ret i32 %12
}

attributes #0 = { nounwind readonly uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-frame-pointer-elim-non-leaf"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }

!0 = metadata !{metadata !"int", metadata !1}
!1 = metadata !{metadata !"omnipotent char", metadata !2}
!2 = metadata !{metadata !"Simple C/C++ TBAA"}
