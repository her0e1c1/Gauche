/*
 * list.c - List related functions
 *
 *   Copyright (c) 2000-2015  Shiro Kawai  <shiro@acm.org>
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. Neither the name of the authors nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LIBGAUCHE_BODY
#include "gauche.h"

/*
 * Classes
 */

static ScmClass *list_cpl[] = {
    SCM_CLASS_STATIC_PTR(Scm_ListClass),
    SCM_CLASS_STATIC_PTR(Scm_SequenceClass),
    SCM_CLASS_STATIC_PTR(Scm_CollectionClass),
    SCM_CLASS_STATIC_PTR(Scm_TopClass),
    NULL
};

SCM_DEFINE_BUILTIN_CLASS(Scm_ListClass, NULL, NULL, NULL, NULL, list_cpl+1);
SCM_DEFINE_BUILTIN_CLASS(Scm_PairClass, NULL, NULL, NULL, NULL, list_cpl);
SCM_DEFINE_BUILTIN_CLASS(Scm_NullClass, NULL, NULL, NULL, NULL, list_cpl);

/*
 * CONSTRUCTOR
 */

ScmObj Scm_Cons(ScmObj car, ScmObj cdr)
{
    ScmPair *z = SCM_NEW(ScmPair);
    /* NB: these ENSURE_MEMs are moved here from vm loop to reduce
       the register pressure there.  In most cases these increases
       just a couple of mask-and-test instructions on the data on
       the register. */
    SCM_FLONUM_ENSURE_MEM(car);
    SCM_FLONUM_ENSURE_MEM(cdr);
    SCM_SET_CAR(z, car);
    SCM_SET_CDR(z, cdr);
    return SCM_OBJ(z);
}

ScmObj Scm_Acons(ScmObj caar, ScmObj cdar, ScmObj cdr)
{
    ScmPair *y = SCM_NEW(ScmPair);
    ScmPair *z = SCM_NEW(ScmPair);
    SCM_SET_CAR(y, caar);
    SCM_SET_CDR(y, cdar);
    SCM_SET_CAR(z, SCM_OBJ(y));
    SCM_SET_CDR(z, cdr);
    return SCM_OBJ(z);
}

ScmObj Scm_List(ScmObj elt, ...)
{
    if (elt == NULL) return SCM_NIL;

    va_list pvar;
    va_start(pvar, elt);
    ScmObj cdr = Scm_VaList(pvar);
    va_end(pvar);
    return Scm_Cons(elt, cdr);
}


ScmObj Scm_Conses(ScmObj elt, ...)
{
    if (elt == NULL) return SCM_NIL;

    va_list pvar;
    va_start(pvar, elt);
    ScmObj cdr = Scm_VaCons(pvar);
    va_end(pvar);
    if (cdr == NULL) return elt;
    else             return Scm_Cons(elt, cdr);
}


ScmObj Scm_VaList(va_list pvar)
{
    ScmObj start = SCM_NIL, cp = SCM_NIL, obj;

    for (obj = va_arg(pvar, ScmObj);
         obj != NULL;
         obj = va_arg(pvar, ScmObj))
    {
        if (SCM_NULLP(start)) {
            start = SCM_OBJ(SCM_NEW(ScmPair));
            SCM_SET_CAR(start, obj);
            SCM_SET_CDR(start, SCM_NIL);
            cp = start;
        } else {
            ScmObj item;
            item = SCM_OBJ(SCM_NEW(ScmPair));
            SCM_SET_CDR(cp, item);
            SCM_SET_CAR(item, obj);
            SCM_SET_CDR(item, SCM_NIL);
            cp = item;
        }
    }
    return start;
}


ScmObj Scm_VaCons(va_list pvar)
{
    Scm_Panic("Scm_VaCons: not implemented");
    return SCM_UNDEFINED;
}

ScmObj Scm_ArrayToList(ScmObj *elts, int nelts)
{
    return Scm_ArrayToListWithTail(elts, nelts, SCM_NIL);
}

ScmObj Scm_ArrayToListWithTail(ScmObj *elts, int nelts, ScmObj tail)
{
    ScmObj h = SCM_NIL, t = SCM_NIL;
    if (elts) {
        for (int i=0; i<nelts; i++) SCM_APPEND1(h, t, *elts++);
    }
    if (!SCM_NULLP(tail)) SCM_APPEND(h, t, tail);
    return h;
}

ScmObj *Scm_ListToArray(ScmObj list, int *nelts, ScmObj *store, int alloc)
{
    int len = Scm_Length(list);
    if (len < 0) Scm_Error("proper list required, but got %S", list);

    ScmObj *array;
    if (store == NULL) {
        array = SCM_NEW_ARRAY(ScmObj, len);
    } else {
        if (*nelts < len) {
            if (!alloc)
                Scm_Error("ListToArray: storage too small");
            array = SCM_NEW_ARRAY(ScmObj, len);
        } else {
            array = store;
        }
    }
    int i = 0;
    for (ScmObj lp=list; i<len; i++, lp=SCM_CDR(lp)) {
        array[i] = SCM_CAR(lp);
    }
    *nelts = len;
    return array;
}

/* cXr stuff */
#define CXR(cname, sname, body)                 \
ScmObj cname (ScmObj obj)                       \
{                                               \
   ScmObj obj2 = obj;                           \
   body                                         \
   return obj2;                                 \
}

#define A                                                       \
   if (!SCM_PAIRP(obj2)) Scm_Error("bad object: %S", obj);      \
   obj2 = SCM_CAR(obj2);

#define D                                                       \
   if (!SCM_PAIRP(obj2)) Scm_Error("bad object: %S", obj);      \
   obj2 = SCM_CDR(obj2);

// car (X=a)
CXR(Scm_Car, "car", A)
CXR(Scm_Cdr, "cdr", D)
CXR(Scm_Caar, "caar", A A)  // ２回Aを実行 (CAR(CAR(obj2)))
CXR(Scm_Cadr, "cadr", D A)
CXR(Scm_Cdar, "cdar", A D)
// cddr (X=dd)
CXR(Scm_Cddr, "cddr", D D)

/*
 * List manipulate routines:
 */

/* Scm_Length
   return length of list in C integer.
   If the argument is a dotted list, return -1.
   If the argument is a circular list, return -2. */

int Scm_Length(ScmObj obj)
{
    ScmObj slow = obj;
    int len = 0;

    for (;;) {
        if (SCM_NULLP(obj)) break;
        if (!SCM_PAIRP(obj)) return SCM_LIST_DOTTED;

        obj = SCM_CDR(obj);
        len++;
        if (SCM_NULLP(obj)) break;
        if (!SCM_PAIRP(obj)) return SCM_LIST_DOTTED;

        obj = SCM_CDR(obj);
        slow = SCM_CDR(slow);
        if (obj == slow) return SCM_LIST_CIRCULAR;
        len++;
    }
    return len;
}

/* Scm_CopyList(list)
 *   Copy toplevel list LIST.  LIST can be improper.
 *   If LIST is not a pair, return LIST itself.
 */

ScmObj Scm_CopyList(ScmObj list)
{
    if (!SCM_PAIRP(list)) return list;

    ScmObj start = SCM_NIL, last = SCM_NIL;
    SCM_FOR_EACH(list, list) {
        SCM_APPEND1(start, last, SCM_CAR(list));
    }
    if (!SCM_NULLP(list)) SCM_SET_CDR(last, list);
    return start;
}

/* Scm_MakeList(len, fill)
 *    Make a list of specified length.
 *    Note that <len> is C-integer.
 */

ScmObj Scm_MakeList(ScmSmallInt len, ScmObj fill)
{
    if (len < 0) {
        Scm_Error("make-list: negative length given: %d", len);
    }
    ScmObj start = SCM_NIL, last = SCM_NIL;
    while (len--) {
        SCM_APPEND1(start, last, fill);
    }
    return start;
}


/* Scm_Append2X(list, obj)
 *    Replace cdr of last pair of LIST for OBJ.
 *    If LIST is not a pair, return OBJ.
 */

ScmObj Scm_Append2X(ScmObj list, ScmObj obj)
{
    ScmObj cp;
    SCM_FOR_EACH(cp, list) {
        if (SCM_NULLP(SCM_CDR(cp))) {
            SCM_SET_CDR(cp, obj);
            return list;
        }
    }
    return obj;
}

/* Scm_Append2(list, obj)
 *   Copy LIST and append OBJ to it.
 *   If LIST is not a pair, return OBJ.
 */

ScmObj Scm_Append2(ScmObj list, ScmObj obj)
{
    if (!SCM_PAIRP(list)) return obj;

    ScmObj start = SCM_NIL, last = SCM_NIL;
    SCM_FOR_EACH(list, list) {
        SCM_APPEND1(start, last, SCM_CAR(list));
    }
    SCM_SET_CDR(last, obj);

    return start;
}

ScmObj Scm_Append(ScmObj args)
{
    ScmObj start = SCM_NIL, last = SCM_NIL, cp;
    SCM_FOR_EACH(cp, args) {
        if (!SCM_PAIRP(SCM_CDR(cp))) {
            if (SCM_NULLP(start)) return SCM_CAR(cp);
            SCM_SET_CDR(last, SCM_CAR(cp));
            break;
        } else if (SCM_NULLP(SCM_CAR(cp))) {
            continue;
        } else if (!SCM_PAIRP(SCM_CAR(cp))) {
            Scm_Error("pair required, but got %S", SCM_CAR(cp));
        } else {
            SCM_APPEND(start, last, Scm_CopyList(SCM_CAR(cp)));
        }
    }
    return start;
}

/* Scm_Reverse2(list, tail)
 *    Reverse LIST, and append TAIL to the result.
 *    If LIST is an improper list, cdr of the last pair is ignored.
 *    If LIST is not a pair, TAIL is returned.
 * Scm_Reverse(list)
 *    Scm_Reverse2(list, SCM_NIL).  Just for the backward compatibility.
 */

// リストをreverse(破壊的でない)
ScmObj Scm_Reverse2(ScmObj list, ScmObj tail)
{
    if (!SCM_PAIRP(list)) return tail;

    ScmPair *p = SCM_NEW(ScmPair);
    SCM_SET_CAR(p, SCM_NIL);
    SCM_SET_CDR(p, tail); //  (NIL . NIL) の状態
    ScmObj result = SCM_OBJ(p);
    ScmObj cp;
    SCM_FOR_EACH(cp, list) {
        SCM_SET_CAR(result, SCM_CAR(cp));
        // 上と同じコードを記述
        p = SCM_NEW(ScmPair);
        // carにNILをセット。（これはreturnのときに無視される）
        SCM_SET_CAR(p, SCM_NIL);
        SCM_SET_CDR(p, result);
        result = SCM_OBJ(p);  // resultは常に先頭
    }
    return SCM_CDR(result);
}

ScmObj Scm_Reverse(ScmObj list)
{
    return Scm_Reverse2(list, SCM_NIL);
}

// リストをreverse(破壊的)
/* Scm_Reverse2X(list, tail)
 *   Return reversed list of LIST.  Pairs in previous LIST is used to
 *   create new list.  TAIL is appended to the result.
 *   If LIST is not a pair, returns TAIL.
 *   If LIST is an improper list, cdr of the last cell is ignored.
 */

ScmObj Scm_Reverse2X(ScmObj list, ScmObj tail)
{
    if (!SCM_PAIRP(list)) return tail;
    ScmObj first, next, result = tail;
    for (first = list; SCM_PAIRP(first); first = next) {
      next = SCM_CDR(first);  // first自体は、リストの要素を順にたどる
        SCM_SET_CDR(first, result);
        result = first;
    }
    return result;
}

ScmObj Scm_ReverseX(ScmObj list)
{
    return Scm_Reverse2X(list, SCM_NIL);
}

/* Scm_ListTail(list, i, fallback)
 * Scm_ListRef(list, i, fallback)
 *    Note that i is C-INTEGER.  If i is out of bound, signal error.
 */

// iに-1を指定すると、負の数なのでその時点でエラー(要素数より多くてもエラー)
// (drop '(1 2 3) 2)  => (3)
ScmObj Scm_ListTail(ScmObj list, ScmSmallInt i, ScmObj fallback)
{
    if (i < 0) goto err;
    ScmSmallInt cnt = i;
    // iが末尾のときもありうるので、O(n)
    while (cnt-- > 0) {
        if (!SCM_PAIRP(list)) goto err;
        list = SCM_CDR(list);  // 先頭POINTERを一つずつ後ろにしていく
    }
    return list;
  err:
    if (SCM_UNBOUNDP(fallback)) Scm_Error("argument out of range: %ld", i);
    return fallback;
}

//  (ref '(1 2 3) 0)  ; 1
ScmObj Scm_ListRef(ScmObj list, ScmSmallInt i, ScmObj fallback)
{
    if (i < 0) goto err;
    for (ScmSmallInt k=0; k<i; k++) {
        if (!SCM_PAIRP(list)) goto err;
        list = SCM_CDR(list);
    }
    // Scm_ListTailと同じだけど、それが返す先頭要素をここで返す
    if (!SCM_PAIRP(list)) goto err;
    return SCM_CAR(list);
  err:
    //  (ref '(1 2 3) 10)  ; error
    if (SCM_UNBOUNDP(fallback)) {
        Scm_Error("argument out of range: %ld", i);
    }
    return fallback;
}

/* Scm_LastPair(l)
 *   Return last pair of (maybe improper) list L.
 *   If L is not a pair, signal error.
 */
// (last-pair '(1 2 3 4))  => '(4)  O(n)
ScmObj Scm_LastPair(ScmObj l)
{
    if (!SCM_PAIRP(l)) Scm_Error("pair required: %S", l);

    ScmObj cp;
    SCM_FOR_EACH(cp, l) {
        ScmObj cdr = SCM_CDR(cp);
        // '(1 2 3) => '(2 3) => '(3)
        // '(3)を返す
        if (!SCM_PAIRP(cdr)) return cp;
    }
    // PAIRが保証されているので、ここにはこないはず
    return SCM_UNDEFINED;       /* NOTREACHED */
}

/* Scm_Memq(obj, list)
 * Scm_Memv(obj, list)
 * Scm_Member(obj, list)
 *    LIST must be a list.  Return the first sublist whose car is obj.
 *    If obj doesn't occur in LIST, or LIST is not a pair, #f is returned.
 */

ScmObj Scm_Memq(ScmObj obj, ScmObj list)
{
  // Memqはeq? (pointer演算) O(n)
  // (memq  'b '(a b c))  ; (b c)
    SCM_FOR_EACH(list, list) if (obj == SCM_CAR(list)) return list;
    return SCM_FALSE;
}

ScmObj Scm_Memv(ScmObj obj, ScmObj list)
{
  // memvは eqv?
  SCM_FOR_EACH(list, list) {
        if (Scm_EqvP(obj, SCM_CAR(list))) return list;
    }
    return SCM_FALSE;
}

ScmObj Scm_Member(ScmObj obj, ScmObj list, int cmpmode)
{
  // modeによって、eq/eqv/equalのいずれかになる
  //  (member 1.0 '(a 1.0 c) eq?)  => #f
    SCM_FOR_EACH(list, list) {
        if (Scm_EqualM(obj, SCM_CAR(list), cmpmode)) return list;
    }
    return SCM_FALSE;
}

/* delete. */
//  (delete 2 '(1 2 3) eq?) => (1 3)
// (delete 2 '(1 2 3 2 3) eq?)  => (1 3 3)
// (こっちは破壊的)
ScmObj Scm_Delete(ScmObj obj, ScmObj list, int cmpmode)
{
    if (SCM_NULLP(list)) return SCM_NIL;

    ScmObj start = SCM_NIL, last = SCM_NIL, cp, prev = list;
    // :TODO: きちんと(delete list)を理解する
    SCM_FOR_EACH(cp, list) {
        if (Scm_EqualM(obj, SCM_CAR(cp), cmpmode)) {
            for (; prev != cp; prev = SCM_CDR(prev))
                SCM_APPEND1(start, last, SCM_CAR(prev));
            prev = SCM_CDR(cp);
        }
    }
    // 等しいものがみつからなかったのでprevが初期状態のままだった
    if (list == prev) return list;

    // 先頭要素だけ削除したので、listは破壊せずにすんだ
    if (SCM_NULLP(start)) return prev;

    if (SCM_PAIRP(prev)) SCM_SET_CDR(last, prev);
    return start;
}

// こっちは一般的なlistのdeleteだと思うが(こっちは破壊的出ない)
ScmObj Scm_DeleteX(ScmObj obj, ScmObj list, int cmpmode)
{
    ScmObj cp, prev = SCM_NIL;
    SCM_FOR_EACH(cp, list) {
        if (Scm_EqualM(obj, SCM_CAR(cp), cmpmode)) {
            if (SCM_NULLP(prev)) {
              list = SCM_CDR(cp);  // 先頭で不要な要素がある場合は, pointerを進める
            } else {
              // そうでない場合は、一つ前のlistの参照を書き換える(node->next = node->next->next)
                SCM_SET_CDR(prev, SCM_CDR(cp));
            }
        } else {
            prev = cp;
        }
    }
    return list;
}


/*
 * assq, assv, assoc
 *    ALIST must be a list of pairs.  Return the first pair whose car
 *    is obj.  If ALIST contains non pair, it's silently ignored.
 */

// assq / assv / assoc はそれぞれeq/eqv/equalだね(比較以外は同じ)
//  (assq 1 '(1 2 3 (1 . a) (1 . b)))  => (1 . a)
ScmObj Scm_Assq(ScmObj obj, ScmObj alist)
{
    if (!SCM_LISTP(alist)) Scm_Error("assq: list required, but got %S", alist);
    ScmObj cp;
    SCM_FOR_EACH(cp,alist) {
        ScmObj entry = SCM_CAR(cp);
        // alist自体は、どんな要素があっても無視されるみたい
        if (!SCM_PAIRP(entry)) continue;
        // 最初に見つかったpairを返すだけみたい
        if (obj == SCM_CAR(entry)) return entry;
    }
    return SCM_FALSE;
}

ScmObj Scm_Assv(ScmObj obj, ScmObj alist)
{
    if (!SCM_LISTP(alist)) Scm_Error("assv: list required, but got %S", alist);
    ScmObj cp;
    SCM_FOR_EACH(cp,alist) {
        ScmObj entry = SCM_CAR(cp);
        if (!SCM_PAIRP(entry)) continue;
        if (Scm_EqvP(obj, SCM_CAR(entry))) return entry;
    }
    return SCM_FALSE;
}

ScmObj Scm_Assoc(ScmObj obj, ScmObj alist, int cmpmode)
{
    if (!SCM_LISTP(alist)) Scm_Error("assoc: list required, but got %S", alist);
    ScmObj cp;
    SCM_FOR_EACH(cp,alist) {
        ScmObj entry = SCM_CAR(cp);
        if (!SCM_PAIRP(entry)) continue;
        if (Scm_EqualM(obj, SCM_CAR(entry), cmpmode)) return entry;
    }
    return SCM_FALSE;
}

/* Assoc-delete */
ScmObj Scm_AssocDelete(ScmObj elt, ScmObj alist, int cmpmode)
{
    if (!SCM_LISTP(alist)) {
        Scm_Error("assoc-delete: list required, but got %S", alist);
    }
    if (SCM_NULLP(alist)) return SCM_NIL;

    ScmObj start = SCM_NIL, last = SCM_NIL, cp, p, prev = alist;
    SCM_FOR_EACH(cp, alist) {
        p = SCM_CAR(cp);
        if (SCM_PAIRP(p)) {
            if (Scm_EqualM(elt, SCM_CAR(p), cmpmode)) {
              // listの再構築(破壊的ではない)
              // pは削除対象
              // その手前までは、削除対象でない
              // prev ~ (cpの一つ前)をリストに追加する
              // このforを実行しない場合が、prevを返す
                for (; prev != cp; prev = SCM_CDR(prev))
                    SCM_APPEND1(start, last, SCM_CAR(prev));
                // prevは、cpの次から
                prev = SCM_CDR(cp);
            }
        }
    }
    if (alist == prev) return alist;  // 一つも削除要素eltがみつからなかった
    // 先頭の要素のみが削除されたので、リストの再構築はせずに、pointerだけ進めた
    if (SCM_NULLP(start)) return prev;
    // APPEND1してない 残りのprevを追加しておく
    if (SCM_PAIRP(prev)) SCM_SET_CDR(last, prev);
    return start;
}

// 同じもの全て削除
// (alist-delete 1 '(1 2 (1 . a) 3))  => (1 2 3)
// (alist-delete 1 '(1 2 (1 . a) 3 (2 . b) (1 . c)))  (1 2 3 (2 . b))
ScmObj Scm_AssocDeleteX(ScmObj elt, ScmObj alist, int cmpmode)
{
    if (!SCM_LISTP(alist)) {
        Scm_Error("assoc-delete!: list required, but got %S", alist);
    }
    ScmObj cp, prev = SCM_NIL;
    SCM_FOR_EACH(cp, alist) {
        ScmObj e = SCM_CAR(cp);
        // pairでないなら削除対象でない
        if (SCM_PAIRP(e)) {
            if (Scm_EqualM(elt, SCM_CAR(e), cmpmode)) {
              // carが一致したので削除する
                if (SCM_NULLP(prev)) {
                  // 先頭なのでpointerを１つ進めるだけ
                    alist = SCM_CDR(cp);
                    continue;
                } else {
                  // cp = prev->next
                  // prev->next = cp->next
                  // つまり
                  // prev->next = prev->next->next
                  // こっちは破壊的操作!
                    SCM_SET_CDR(prev, SCM_CDR(cp));
                }
            }
        }
        prev = cp;
    }
    return alist;
}

/* DeleteDuplicates.  preserve the order of original list.   N^2 algorithm */

ScmObj Scm_DeleteDuplicates(ScmObj list, int cmpmode)
{
    ScmObj result = SCM_NIL, tail = SCM_NIL, lp;
    SCM_FOR_EACH(lp, list) {
      // Scm_MemberもO(n)なので、N^2かかる
      // 新しいlistのresultに要素がなければ、追加していく
        if (SCM_FALSEP(Scm_Member(SCM_CAR(lp), result, cmpmode))) {
            SCM_APPEND1(result, tail, SCM_CAR(lp));
        }
    }
    if (!SCM_NULLP(lp) && !SCM_NULLP(tail)) SCM_SET_CDR(tail, lp);
    return result;
}

ScmObj Scm_DeleteDuplicatesX(ScmObj list, int cmpmode)
{
    ScmObj lp;
    // (delete-duplicates '(1 2 3 1 1 2)) =>  (1 2 3)
    SCM_FOR_EACH(lp, list) {
        ScmObj obj = SCM_CAR(lp);
        // (cdr lp) を破壊的に変更(こっちも、deletexがO(n)なので、N^2)
        ScmObj tail = Scm_DeleteX(obj, SCM_CDR(lp), cmpmode);

        // いずれにしても、(lp tail)の関係になるように調整
        if (SCM_CDR(lp) != tail) SCM_SET_CDR(lp, tail);
    }
    return list;
}

/*
 * Monotonic Merge
 *
 *  Merge lists, keeping the order of elements (left to right) in each
 *  list.   If there's more than one way to order an element, choose the
 *  first one appears in the given list of lists.
 *  Returns SCM_FALSE if the lists are inconsistent to be ordered
 *  in the way.
 *
 *  START is an item of the starting point.  It is inserted into the result
 *  first.  SEQUENCES is a list of lists describing the order of preference.
 *
 *  The algorithm is used in C3 linearization of class precedence
 *  calculation, described in the paper
 *    http://www.webcom.com/~haahr/dylan/linearization-oopsla96.html.
 *  Since the algorithm is generally useful, I implement the core routine
 *  of the algorithm here.
 *
 *  TODO at 1.0: I noticed START argument acutally isn't used in the
 *  algorithm at all.  We can drop it and the caller can just say
 *  Scm_Cons(start, Scm_MonotonicMerge(sequences)).   We can't change it
 *  now because of ABI compatibility, but it will be nice to do so when
 *  releasing 1.0.
 */

/* DEPRECATED 1.0 */
ScmObj Scm_MonotonicMerge(ScmObj start, ScmObj sequences)
{
    ScmObj r = Scm_MonotonicMerge1(sequences);
    if (!SCM_FALSEP(r)) r = Scm_Cons(start, r);
    return r;
}

/* WILL RENAME IN 1.0 */
ScmObj Scm_MonotonicMerge1(ScmObj sequences)
{
    ScmObj result = SCM_NIL;
    int nseqs = Scm_Length(sequences);
    if (nseqs < 0) Scm_Error("bad list of sequences: %S", sequences);
    ScmObj *seqv = SCM_NEW_ARRAY(ScmObj, nseqs);
    for (ScmObj *sp=seqv;
         SCM_PAIRP(sequences);
         sp++, sequences=SCM_CDR(sequences)) {
        *sp = SCM_CAR(sequences);
    }

    for (;;) {
        /* have we consumed all the inputs? */
        ScmObj *sp;
        for (sp=seqv; sp<seqv+nseqs; sp++) {
            if (!SCM_NULLP(*sp)) break;
        }
        if (sp == seqv+nseqs) return Scm_ReverseX(result);

        /* select candidate */
        ScmObj next = SCM_FALSE;
        for (sp = seqv; sp < seqv+nseqs; sp++) {
            if (!SCM_PAIRP(*sp)) continue;
            ScmObj h = SCM_CAR(*sp);
            ScmObj *tp;
            for (tp = seqv; tp < seqv+nseqs; tp++) {
                if (!SCM_PAIRP(*tp)) continue;
                if (!SCM_FALSEP(Scm_Memq(h, SCM_CDR(*tp)))) {
                    break;
                }
            }
            if (tp != seqv+nseqs) continue;
            next = h;
            break;
        }

        if (SCM_FALSEP(next)) return SCM_FALSE; /* inconsistent */

        /* move the candidate to the result */
        result = Scm_Cons(next, result);
        for (sp = seqv; sp < seqv+nseqs; sp++) {
            if (SCM_PAIRP(*sp) && SCM_EQ(next, SCM_CAR(*sp))) {
                *sp = SCM_CDR(*sp);
            }
        }
    }
    /* NOTREACHED */
}

/*
 * Pair attributes
 */

//  (pair? (cons 1 2))  => #t
// (extended-cons a b) ; gauche.internalからロード
// (piar-attributes a)  ; こっちはextendでなくてもよい
ScmObj Scm_PairAttr(ScmPair *pair)
{
  // attributesを属性にもつ
    if (SCM_EXTENDED_PAIR_P(pair)) {
        return SCM_EXTENDED_PAIR(pair)->attributes;
    } else {
        return SCM_NIL;
    }
}

ScmObj Scm_ExtendedCons(ScmObj car, ScmObj cdr)
{
    ScmExtendedPair *xp = SCM_NEW(ScmExtendedPair);
    xp->car = car;
    xp->cdr = cdr;
    // それぞれのconsにattrsがあるみたい
    xp->attributes = SCM_NIL;  // 要するにリストを入れておける(alistかな基本)
    return SCM_OBJ(xp);
}

ScmObj Scm_PairAttrGet(ScmPair *pair, ScmObj key, ScmObj fallback)
{
    if (!SCM_EXTENDED_PAIR_P(pair)) {
        goto fallback;
    }

    // alistを捜索してるだけね.
    ScmObj p = Scm_Assq(key, SCM_EXTENDED_PAIR(pair)->attributes);
    if (SCM_PAIRP(p)) return SCM_CDR(p);
  fallback:
    if (fallback == SCM_UNBOUND)
        Scm_Error("No value associated with key %S in pair attributes of %S",
                  key, SCM_OBJ(pair));
    return fallback;
}

ScmObj Scm_PairAttrSet(ScmPair *pair, ScmObj key, ScmObj value)
{
    if (!SCM_EXTENDED_PAIR_P(pair)) {
      // (pair-attributes-set! a 'key value)  a が不適切な場合
        Scm_Error("Cannot set pair attribute (%S) to non-extended pair: %S",
                  key, SCM_OBJ(pair));
    }

    ScmObj p = Scm_Assq(key, SCM_EXTENDED_PAIR(pair)->attributes);
    if (SCM_PAIRP(p)) SCM_SET_CDR(p, value);  //すでに存在していれば更新
    else SCM_EXTENDED_PAIR(pair)->attributes
           // (acons k v alist) して新しいデータを格納
        = Scm_Acons(key, value, SCM_EXTENDED_PAIR(pair)->attributes);
    // 未定義を返しておく
    return SCM_UNDEFINED;
}


