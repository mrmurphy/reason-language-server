Printexc.record_backtrace(true);

exception Expect(string);

type stringExpect = {toEqual: (~message: string=?, string) => unit};
type intExpect = {toEqual: (~message: string=?, int) => unit};
type expect = {
  string: string => stringExpect,
  int: int => intExpect,
};

let expect = {
  string: actual => {
    toEqual: (~message=?, expected) =>
      if (actual != expected) {
        failwith(
          Printf.sprintf(
            "Expected %s to equal %s : %s",
            actual,
            expected,
            switch (message) {
            | None => ""
            | Some(m) => m
            },
          ),
        );
      },
  },
  int: actual => {
    toEqual: (~message=?, expected) =>
      if (actual != expected) {
        failwith(
          Printf.sprintf(
            "Expected %d to equal %d : %s",
            actual,
            expected,
            switch (message) {
            | None => ""
            | Some(m) => m
            },
          ),
        );
      },
  },
};

type lifecycle('parentEnv, 'allEnv, 'eachEnv) = {
  beforeAll: 'parentEnv => 'allEnv,
  beforeEach: 'allEnv => 'eachEnv,
  afterEach: 'eachEnv => unit,
  afterAll: 'allEnv => unit,
};

// let composeLifecycles =
//     (
//       parent: lifecycle('grandParentEnv, 'parentAll, 'parentEach),
//       child: lifecycle('parentAll, 'allEnv, 'eachEnv),
//     ): lifecycle('grandParentEnv, ('parentAll, 'allEnv), ('parentEach, 'eachEnv)) => {
//   beforeAll: env => {
//     let parentAll = parent.beforeAll(env);
//     let childAll = child.beforeAll(parentAll);
//     (parentAll, childAll)
//   },
//   beforeEach: ((parentAll, childAll)) => {
//     let parentEach = parent.beforeEach(parentAll);
//     let childEach = child.beforeEach(childAll);
//     (parentEach, childEach)
//   },
//   afterEach: ((parentEach, childEach)) => {
//     parent.afterEach(parentEach);
//     child.afterEach(childEach);
//   },
//   afterAll: ((parentAll, childAll)) => {
//     parent.afterAll(parentAll);
//     child.afterAll(childAll);
//   }
// };

let plainLc = {beforeAll: x => x, beforeEach: x => x, afterEach: _ => (), afterAll: _ => ()};

let rootLc = {beforeAll: () => (), beforeEach: () => (), afterEach: () => (), afterAll: () => ()};

type describeArgs('parentEnv, 'allEnv, 'eachEnv) = {
  it: (string, testArgs('eachEnv) => unit) => unit,
  inner: suite('parentEnv, 'allEnv, 'eachEnv),
}
and describe('parentEnv) = {
  plain: (string, describeArgs('parentEnv, 'parentEnv, 'parentEnv) => unit) => unit,
  withLifecycle:
    'allEnv 'eachEnv.
    (
      string,
      lifecycle('parentEnv, 'allEnv, 'eachEnv),
      describeArgs('parentEnv, 'allEnv, 'eachEnv) => unit
    ) =>
    unit,

}
and describeWithOptions('parentEnv) = {
  plain: (string, describeArgs('parentEnv, 'parentEnv, 'parentEnv) => unit) => unit,
  withLifecycle:
    'allEnv 'eachEnv.
    (
      string,
      lifecycle('parentEnv, 'allEnv, 'eachEnv),
      describeArgs('parentEnv, 'allEnv, 'eachEnv) => unit
    ) =>
    unit,

  skip: describe('parentEnv),
}
and testArgs('env) = {
  expect,
  ctx: 'env,
}
and child('allEnv, 'eachEnv) =
  | Test(string, testArgs('eachEnv) => unit)
  | Suite(lockedSuite('allEnv))
and lockedSuite('parentEnv) =
  | LockedSuite(suite('parentEnv, 'allEnv, 'eachEnv)): lockedSuite('parentEnv)
and suite('parentEnv, 'allEnv, 'eachEnv) = {
  name: string,
  skipped: bool,
  lc: lifecycle('parentEnv, 'allEnv, 'eachEnv),
  mutable children: list(child('allEnv, 'eachEnv)),
};

let suite = suite => Suite(LockedSuite(suite));

let describeArgs = inner => {
  it: (name, body) => inner.children = [Test(name, body), ...inner.children],
  inner,
};

let makeDescribe = parent => {
  let describeWithOptions = {
    plain: (name, body) => {
      let inner = {name, lc: plainLc, skipped: false, children: []};
      body(describeArgs(inner));
      parent.children = [
        suite(inner),
        ...parent.children,
      ];
    },
    withLifecycle: (name, lc, body) => {
      let inner = {name, lc, skipped: false, children: []}
      body(describeArgs(inner));
      parent.children = [
        suite(inner),
        ...parent.children,
      ];
    },
    skip: {
      plain: (name, body) => {
        let inner = {name, lc: plainLc, skipped: true, children: []};
        body(describeArgs(inner));
        parent.children = [
          suite(inner),
          ...parent.children,
        ];
      },
      withLifecycle: (name, lc, body) => {
        let children = ref([]);
        let inner = {name, lc, skipped: true, children: []};
        body(describeArgs(inner));
        parent.children = [
          suite(inner),
          ...parent.children,
        ];
      },
    },
  };
  describeWithOptions;
};

let rootSuite = {name: "Root", lc: rootLc, skipped: false, children: []};
let describe = makeDescribe(rootSuite);

let beforeAll = beforeAll => {
  beforeAll,
  beforeEach: x => x,
  afterEach: _ => (),
  afterAll: () => (),
};

let beforeEach = beforeEach => {
  beforeAll: () => (),
  beforeEach,
  afterEach: _ => (),
  afterAll: () => (),
};

let beforeAfterEach = (before, after) => {
  beforeAll: () => (),
  beforeEach: before,
  afterEach: after,
  afterAll: () => (),
};

describe.withLifecycle(
  "A",
  {
    beforeAll: () => {print_endline("A before all"); 10},
    beforeEach: num => {print_endline("A before each"); string_of_int(num)},
    afterEach: (s: string) => print_endline("A after each" ++ s),
    afterAll: (x: int) => print_endline("A after all"),
  },
  ({it, inner}) => {
  it("a1", ({expect, ctx}) => {
    expect.string(ctx).toEqual("10")
  });
  let describe = makeDescribe(inner);
  describe.withLifecycle(
    "B",
    {
      beforeAll: aAll => {print_endline("B before all"); aAll * 2},
      beforeEach: num => {print_endline("B before each"); string_of_int(num)},
      afterEach: (s: string) => print_endline("B after each " ++ s),
      afterAll: (x: int) => print_endline("B after all"),
    },
    ({it}) => {
    it("b1", ({expect, ctx}) => {
      expect.string(ctx).toEqual("20")
    })
    it("b2", ({expect, ctx}) => {
      expect.string(ctx).toEqual("10")
    })
  });
  ()
});



/*

 A
 - a1
 - B
   - b1
   - b2

 describe.withLifecycle("A", lc, ({describe, test}) => {
   test.it("a1", ({expect, ctx}) => {

   });

   describe.withLifecycle("B", {
     beforeAll: parentCtx => ctx,
     beforeEach: // it's fine actually
   })
 })

 aAll = A.beforeAll()
   aEach = A.beforeEach(aAll)
     a1(aEach)
   A.afterEach(aEach)
   bAll = B.beforeAll(aAll)
     aEach = A.beforeEach(aAll)
     bEach = B.beforeEach(bAll) // also aEach?
       b1(bEach)
     B.afterEach(bEach)
     A.afterEach(aEach)
     aEach = A.beforeEach(aAll)
     bEach = B.beforeEach(bEach) // also aEach
       b2(bEach)
     B.afterEach(bEach)
     A.afterEach(aEach)
   B.afterAll(bAll)
 A.afterAll(aAll)

 */

type suiteResult =
  | BeforeError(string)
  | Results({
      afterErr: option(string),
      tests: list(childResult),
    })
and testResult =
  | BeforeEachError(string)
  | TestResult({
      after: option(string),
      err: option(string),
    })
and childResult =
  | SuiteResult({
      name: string,
      result: suiteResult,
    })
  | ChildTestResult({
      name: string,
      result: testResult,
    });

let catcher = fn =>
  switch (fn()) {
  | exception (Expect(err)) => Error("Expectation error: " ++ err)
  | exception (Failure(err)) => Error("Failure: " ++ err)
  | exception exn => Error("Other error: " ++ Printexc.to_string(exn))
  | x => Ok(x)
  };

// let runTest = (name, body, suite) => {

// }

type parentArgs('all, 'each) = {
  v: 'all,
  beforeEach: unit => 'each,
  afterEach: 'each => unit,
};

type event =
  | SuiteStart(list(string))
  | SuiteEnd(list(string), suiteResult)
  | TestStart(string, list(string))
  | TestEnd(string, list(string), testResult)

let showTrail = trail => List.rev(trail) |> String.concat(":");

let showEvent = fun
  | SuiteStart(trail) => ">> " ++ showTrail(trail)
  | SuiteEnd(trail, result) => "<< " ++ showTrail(trail)
  | TestStart(name, trail) => "#> [" ++ name ++ "] " ++ showTrail(trail)
  | TestEnd(name, trail, testResult) => "#< [" ++ name ++ "] " ++ showTrail(trail)


let rec runSuite:
  type parentEnv parentEach.
    (event => unit, list(string), lockedSuite(parentEnv), parentArgs(parentEnv, parentEach)) => suiteResult =
  (
    report,
    trail,
    LockedSuite(suite): lockedSuite(parentEnv),
    parent: parentArgs(parentEnv, parentEach),
  ) => {
    let trail = [suite.name, ...trail];
    report(SuiteStart(trail));
    let result = switch (catcher(() => suite.lc.beforeAll(parent.v))) {
    | Error(err) => BeforeError(err)
    | Ok(beforeAll) =>
      let tests =
        suite.children->List.rev
        ->Belt.List.map(child =>
            switch (child) {
            | Suite(LockedSuite({name}) as childSuite) =>
              SuiteResult({
                name,
                result:
                  runSuite(
                    report,
                    trail,
                    childSuite,
                    {
                      v: beforeAll,
                      beforeEach: () => {
                        let parentEach = parent.beforeEach();
                        switch (suite.lc.beforeEach(beforeAll)) {
                        | exception exn =>
                          try(parent.afterEach(parentEach)) {
                          | _ => ()
                          };
                          raise(exn);
                        | before => (parentEach, before)
                        };
                      },
                      afterEach: ((parentEach, each)) => {
                        let after =
                          switch (suite.lc.afterEach(each)) {
                          | exception exn => Some(exn)
                          | () => None
                          };
                        let parent =
                          switch (parent.afterEach(parentEach)) {
                          | exception exn => Some(exn)
                          | () => None
                          };
                        switch (parent, after) {
                        | (Some(exn), _) => raise(exn)
                        | (_, Some(exn)) => raise(exn)
                        | _ => ()
                        };
                      },
                    },
                  ),
              })
            | Test(name, body) =>
              report(TestStart(name, trail));
              let result = {
                  switch (catcher(parent.beforeEach)) {
                  | Error(err) => BeforeEachError(err)
                  | Ok(parentEach) =>
                    switch (catcher(() => suite.lc.beforeEach(beforeAll))) {
                    | Error(err) => BeforeEachError(err)
                    | Ok(ctx) =>
                      let err =
                        switch (catcher(() => body({expect, ctx}))) {
                        | Error(err) => Some(err)
                        | Ok () => None
                        };
                      TestResult({
                        err,
                        after: {
                          let afterErr = catcher(() => suite.lc.afterEach(ctx));
                          let parentErr = catcher(() => parent.afterEach(parentEach));
                          switch (parentErr, afterErr) {
                          | (Error(err), _) => Some(err)
                          | (_, Error(err)) => Some(err)
                          | _ => None
                          };
                        },
                      });
                    }
                  };
                }
              report(TestEnd(name, trail, result));
              ChildTestResult({
                name,
                result,
              });
            }
          );
      Results({
        tests,
        afterErr:
          switch (catcher(() => suite.lc.afterAll(beforeAll))) {
          | Error(err) => Some(err)
          | _ => None
          },
      });
    // TODO have a way to report both errors
    };
    report(SuiteEnd(trail, result));
    result
  };

let report = x => {
  print_endline(showEvent(x))
};
runSuite(report, [], LockedSuite(rootSuite), {
  v: (),
  beforeEach: () => {
    print_endline("root before each")
  },
  afterEach: () => {
    print_endline("root after each")
  }
});