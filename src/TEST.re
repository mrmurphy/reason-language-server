type personObj = {
  .
  "surname": string,
  "age": int,
};

module Person {
[@deriving fields]
type t = {
  surname: string,
  age: int,
};
};

let sean = {Person.surname: "coolguy", age: 9000};
let gandalf = {"surname": "the grey", "age": 9000};

// Js.log(gandalf)
// Js.log(sean.);
();