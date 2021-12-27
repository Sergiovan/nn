pub enum Ast {
    IDENTIFIER(String),
    TYPE(TypeAst),
    BOX(BoxAst),
    BLOCK(Vec<Ast>),
    PROGRAM(Vec<Ast>),
    IMPORT(String),
    USING{this: String, r#as: String},
    DEF(Box<Ast>),
    STRUCT{name: String, stmts: Vec<Ast>},
    UNION{name: String, stmts: Vec<Ast>},
    ENUM{name: String, items: Vec<String>},
    FUN{name: String, params: FunctionDefParams, body: Option<Vec<Ast>>},
    VAR{r#box: BoxAst, name: String, value: Option<Box<Ast>>},
    EXTERN{r#type: String, decls: Vec<Ast>},
    IF{conds: Vec<Ast>, bodies: Vec<Ast>},
    WHILE{cond: Box<Ast>, body: Vec<Ast>},
    LOOP{body: Vec<Ast>},
    MATCH{value: Box<Ast>, cases: Vec<Ast>},
    CASE{pattern: Box<Ast>, body: Vec<Ast>},
    RETURN(Box<Ast>),
    GOTO(String),
    LABEL(String),
    BREAK,
    CONTINUE,
    ELSE,
    PREUNARY(PreUnaryOp, Box<Ast>),
    POSTUNARY(PostUnaryOp, Box<Ast>),
    BINARY{op: InfixOp, lhs: Box<Ast>, rhs: Box<Ast>},
    FUNCALL{params: Vec<Ast>},
    ACCESS(Box<Ast>),
    SIMPLELITERAL(SimpleLiteralAst),
    STRUCTLITERAL{values: Vec<Ast>},
}

pub enum SimpleLiteralAst {
    INTEGER(u64),
    FLOAT(f64),
    STRING(String),
    BOOLEAN(bool),
    NULL
}

pub enum PreUnaryOp {
    INCREMENT, NEGATE, DECREMENT, 
    BNOT, LNOT, DEREFERENCE, ADDRESSOF, 
    SIZEOF
}

pub enum PostUnaryOp {
    INCREMENT, DECREMENT, 
}

pub enum InfixOp {
    ADD, SUBTRACT, EQUALS, NOTEQUALS, LT, LTEQ, GT, GTEQ,
    SHR, SHL, ACCESS, MODULO, BXOR, LXOR, BAND, LAND, BOR, LOR,
    MULTIPLY, DIVIDE, 
}

pub enum TypeAst {
    U0, U1, U8, U16, U32, U64,
    S8, S16, S32, S64, F32, F64, 
    PTR(Box<BoxAst>, Box<TypeAst>), FUN(Box<FunctionTypeAst>), IDENTIFIER(String)
}

pub enum BoxAst {
    VAR, CONST
}

pub struct FunctionTypeAst {
    params: Vec<TypeAst>,
    r#return: TypeAst
}

pub struct FunctionDefParams {
    params: Vec<(String, TypeAst)>,
    r#return: TypeAst
}