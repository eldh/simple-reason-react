type reactClass;

type jsProps;

type reactElement;

[@bs.val] external null: reactElement = "null";

external string: string => reactElement = "%identity";

type reactRefCurrent = Js.nullable(reactElement);

[@bs.deriving abstract]
type reactRef = {
  [@bs.as "current"]
  currentRef: reactRefCurrent,
};

external array: array(reactElement) => reactElement = "%identity";

external __currentToJsObj__: reactRefCurrent => Js.nullable(Js.t({..})) =
  "%identity";

let currentRefGetJs = r =>
  r->currentRefGet->__currentToJsObj__->Js.Nullable.toOption;

[@bs.splice] [@bs.val] [@bs.module "react"]
external createElement:
  (reactClass, ~props: Js.t({..})=?, array(reactElement)) => reactElement =
  "createElement";

[@bs.val] [@bs.module "react"]
external createRef: unit => reactRef = "createRef";

[@bs.val] [@bs.module "react"]
external reactForwardRef: (. 'a) => 'a = "forwardRef";

[@bs.val] [@bs.module "react"] external reactMemo: (. 'a) => 'a = "memo";

[@bs.splice] [@bs.module "react"]
external cloneElement:
  (reactElement, ~props: Js.t({..})=?, array(reactElement)) => reactElement =
  "cloneElement";

[@bs.val] [@bs.module "react"]
external createElementVerbatim: 'a = "createElement";

let createDomElement = (s, ~props, children) => {
  let vararg =
    [|Obj.magic(s), Obj.magic(props)|] |> Js.Array.concat(children);
  /* Use varargs to avoid warnings on duplicate keys in children */
  Obj.magic(createElementVerbatim)##apply(Js.Nullable.null, vararg);
};

[@bs.val] external magicNull: 'a = "null";

type reactClassInternal = reactClass;

type renderNotImplemented =
  | RenderNotImplemented;

/***
 * Elements are what JSX blocks become. They represent the *potential* for a
 * component instance and state to be created / updated. They are not yet
 * instances.
 */
type element =
  | Element(component): element
and jsPropsToReason('jsProps) = (. 'jsProps) => component
and jsElementWrapped =
  option(
    (~key: Js.nullable(string), ~ref: option(reactRef)) => reactElement,
  )
and component = {
  debugName: string,
  reactClassInternal,
  jsElementWrapped,
  render: option(reactRef) => reactElement,
};

let anyToUnit = _ => ();

let anyToTrue = _ => true;

let renderDefault = _ => string("RenderNotImplemented");
let convertPropsIfTheyreFromJs = (props, debugName) => {
  let props = Obj.magic(props);
  switch (Js.Nullable.toOption(props##reasonProps)) {
  | Some(props) => props
  | None =>
    raise(
      Invalid_argument(
        "A JS component called the Reason component " ++ debugName,
      ),
    )
  };
};

let createClass = (~memo=false, ~forwardRef=false, debugName: string) => {
  let renderWrapper =
    switch (memo, forwardRef) {
    | (true, true) => (
        (. render) => reactMemo(. reactForwardRef(. render))
      )
    | (false, true) => reactForwardRef
    | (true, false) => reactMemo
    | (false, false) => ((. render) => render)
    };
  ReasonReactOptimizedCreateClass.createClass(. {
    "displayName": debugName,
    "render":
      renderWrapper(.(props, ref: option(reactRef)) => {
        let convertedReasonProps =
          convertPropsIfTheyreFromJs(props, debugName);
        let Element(created) = Obj.magic(convertedReasonProps);
        let component = created;
        component.render(ref);
      }),
  });
};

let component = (~memo=false, ~forwardRef=false, debugName) => {
  let componentTemplate = {
    reactClassInternal: createClass(~memo, ~forwardRef, debugName),
    debugName,
    render: renderDefault,
    jsElementWrapped: None,
  };
  componentTemplate;
};

let statelessComponent = component;

let element =
    (
      ~key: string=Obj.magic(Js.Nullable.undefined),
      ~ref: option(reactRef)=None,
      component: component,
    ) => {
  let element = Element(component);
  switch (component.jsElementWrapped) {
  | Some(jsElementWrapped) =>
    jsElementWrapped(~key=Js.Nullable.return(key), ~ref)
  | None =>
    createElement(
      component.reactClassInternal,
      ~props={"key": key, "ref": ref, "reasonProps": element},
      [||],
    )
  };
};

let wrapReasonForJs =
    (~component, jsPropsToReason: jsPropsToReason('jsProps)) => {
  let jsPropsToReason: jsPropsToReason(jsProps) =
    (. jsProps) => (Obj.magic(jsPropsToReason))(. jsProps);
  Obj.magic(component.reactClassInternal)##prototype##jsPropsToReason
  #= Some(jsPropsToReason);
  component.reactClassInternal;
};

module WrapProps = {
  /* We wrap the props for reason->reason components, as a marker that "these props were passed from another
     reason component" */
  let wrapProps =
      (
        ~reactClass,
        ~props,
        children,
        ~key: Js.nullable(string),
        ~ref: option(reactRef),
      ) => {
    let props =
      Js.Obj.assign(
        Js.Obj.assign(Js.Obj.empty(), Obj.magic(props)),
        {"ref": ref, "key": key},
      );
    let varargs =
      [|Obj.magic(reactClass), Obj.magic(props)|]
      |> Js.Array.concat(Obj.magic(children));
    /* Use varargs under the hood */
    Obj.magic(createElementVerbatim)##apply(Js.Nullable.null, varargs);
  };
  let dummyInteropComponent = component("Interop");

  let wrapJsForReason = (~reactClass, ~props, children): component => {
    let jsElementWrapped = Some(wrapProps(~reactClass, ~props, children));
    {...dummyInteropComponent, jsElementWrapped};
  };
};
let wrapJsForReason = WrapProps.wrapJsForReason;

[@bs.module "react"] external fragment: 'a = "Fragment";

module Router = {
  [@bs.get] external location: Dom.window => Dom.location = "";
  [@bs.send] [@bs.send]
  /* actually the cb is Dom.event => unit, but let's restrict the access for now */
  external addEventListener:
    (Dom.window, [@bs.string] [ | `popstate], Dom.event => unit) => unit =
    "";
  [@bs.send]
  external removeEventListener:
    (Dom.window, [@bs.string] [ | `popstate], Dom.event => unit) => unit =
    "";
  [@bs.send] external dispatchEvent: (Dom.window, Dom.event) => unit = "";
  [@bs.get] external pathname: Dom.location => string = "";
  [@bs.get] external hash: Dom.location => string = "";
  [@bs.get] external search: Dom.location => string = "";
  [@bs.send]
  external pushState:
    (Dom.history, [@bs.as {json|null|json}] _, [@bs.as ""] _, ~href: string) =>
    unit =
    "";
  [@bs.new]
  external makeEvent: ([@bs.string] [ | `popstate]) => Dom.event = "Event";
  /* if we ever roll our own parser in the future, make sure you test all url combinations
     e.g. foo.com/?#bar
     */
  /* sigh URLSearchParams doesn't work on IE11, edge16, etc. */
  /* actually you know what, not gonna provide search for now. It's a mess.
     We'll let users roll their own solution/data structure for now */
  let path = () =>
    switch ([%external window]) {
    | None => []
    | Some((window: Dom.window)) =>
      switch (window |> location |> pathname) {
      | ""
      | "/" => []
      | raw =>
        /* remove the preceeding /, which every pathname seems to have */
        let raw = Js.String.sliceToEnd(~from=1, raw);
        /* remove the trailing /, which some pathnames might have. Ugh */
        let raw =
          switch (Js.String.get(raw, Js.String.length(raw) - 1)) {
          | "/" => Js.String.slice(~from=0, ~to_=-1, raw)
          | _ => raw
          };
        raw |> Js.String.split("/") |> Array.to_list;
      }
    };
  let hash = () =>
    switch ([%external window]) {
    | None => ""
    | Some((window: Dom.window)) =>
      switch (window |> location |> hash) {
      | ""
      | "#" => ""
      | raw =>
        /* remove the preceeding #, which every hash seems to have.
           Why is this even included in location.hash?? */
        raw |> Js.String.sliceToEnd(~from=1)
      }
    };
  let search = () =>
    switch ([%external window]) {
    | None => ""
    | Some((window: Dom.window)) =>
      switch (window |> location |> search) {
      | ""
      | "?" => ""
      | raw =>
        /* remove the preceeding ?, which every search seems to have. */
        raw |> Js.String.sliceToEnd(~from=1)
      }
    };
  let push = path =>
    switch ([%external history], [%external window]) {
    | (None, _)
    | (_, None) => ()
    | (Some((history: Dom.history)), Some((window: Dom.window))) =>
      pushState(history, ~href=path);
      dispatchEvent(window, makeEvent(`popstate));
    };
  type url = {
    path: list(string),
    hash: string,
    search: string,
  };
  type watcherID = Dom.event => unit;
  let url = () => {path: path(), hash: hash(), search: search()};
  /* alias exposed publicly */
  let dangerouslyGetInitialUrl = url;
  let watchUrl = callback =>
    switch ([%external window]) {
    | None => (_ => ())
    | Some((window: Dom.window)) =>
      let watcherID: watcherID = (_event => callback(url()));
      addEventListener(window, `popstate, watcherID);
      watcherID;
    };
  let unwatchUrl = watcherID =>
    switch ([%external window]) {
    | None => ()
    | Some((window: Dom.window)) =>
      removeEventListener(window, `popstate, watcherID)
    };

  let useUrl = () => {
    let (url, setUrl) = ReactHooks.useState(url());
    ReactHooks.useEffect(
      () => {
        let handleChange = url => setUrl(. url);
        let listener = watchUrl(handleChange);
        Some((.) => unwatchUrl(listener));
      },
      [||],
    );
    url;
  };
};