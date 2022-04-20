use std::vec::IntoIter;
use std::fs;
use std::io::Error;
use std::iter::Peekable;

#[allow(dead_code)]
#[derive(PartialEq, Clone, Debug)]
pub enum TokenData {
    IDEN(String),
    INT(u64),
    FLOAT(f64),
    CHR(char),
    STR(String),
    NOTE(String),
    COMMENT{text: String, block: bool},
    U0, U1, U8, U16, U32, U64,
    S8, S16, S32, S64, F32, F64, 
    STRUCT, UNION, ENUM, FUN,

    VAR, CONST, DEF,

    IF, THEN, ELSE, WHILE, LOOP, MATCH,
    CASE, BREAK, CONTINUE, GOTO, LABEL,

    SIZEOF, AS,

    TRUE, FALSE, NULL,

    IMPORT, FROM, EXTERN, USING,

    C8, C16, C32, E32, ANY, INFER, 
    TUPLE, TYPE, 

    LET, REF, VOLAT, INLINE, ALIGN,

    FOR, RAISE, DEFER, TRY, CATCH, 
    YIELD, ASYNC, AWAIT, NEW, DELETE,
    
    UNDERSCORE, THIS, STATIC, DYNAMIC,
    NAMESPACE, ASM, EXPORT, OPERATOR,
    IN, TYPEOF, AND, OR, NOT, GEN, 
    LAMBDA, CTE,

    PLUS, INCREMENT, MINUS, DECREMENT, EQ,
    EQEQ, NOTEQ, GT, SHR, LT, SHL, GTEQ, SLARROW, SRARROW,
    
    OPAREN, CPAREN, OTUPLE,
    OBRACE, CBRACE, OSTRUCT,
    OBRACK, CBRACK, OARRAY,

    COMMA, DOT, CONCAT, SPREAD,
    DOTPAREN, DOTBRACE, DOTBRACK,
    COLON, BIND, LARROW, RARROW,
    BANG, LNOT, AMPERSAND, LAND, VBAR, LOR, CARET, LXOR,
    AT, HASH, PERCENT, BITSET, BITCLEAR, BITFLIP, BITCHECK,
    ASTERISK, SQUIGGLY, SEMICOLON, SLASH, QM, 
    
    PLUSEQ, MINUSEQ, ASTERISKEQ, SLASHEQ, PERCENTEQ,
    SHLEQ, SHREQ, ANDEQ, OREQ, XOREQ, BITSETEQ, BITCLEAREQ, BITFLIPEQ,
    CONCATEQ, 

    DIAMOND, WLARROW, WRARROW, LPIPE, RPIPE, NULLISH, BACKTICK,

    NOTEOPEN,

    DEFAULT,
    ERROR(String),
    LINEBREAK
}

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct Token {
    pub t: TokenData, // Type
    l: u32, // Line
    c: u32, // Column
    p: u32, // Position
}

impl std::fmt::Display for Token {
    fn fmt(&self, fmt: &mut std::fmt::Formatter) -> std::fmt::Result {
        match &self.t {
            TokenData::IDEN(i) => write!(fmt, "{}", i),
            TokenData::INT(u) => write!(fmt, "{}", u),
            TokenData::FLOAT(f) => write!(fmt, "{}", f),
            TokenData::CHR(c) => write!(fmt, "{}", c),
            TokenData::STR(s) => write!(fmt, "\"{}\"", s),
            TokenData::NOTE(n) => write!(fmt, "${}", n),
            TokenData::COMMENT{text: c, block: true} => write!(fmt, "/*{}*/", c),
            TokenData::COMMENT{text: c, block: false} => write!(fmt, "//{}\n", c),
            TokenData::U0 => write!(fmt, "u0"),
            TokenData::U1 => write!(fmt, "u1"),
            TokenData::U8 => write!(fmt, "u8"),
            TokenData::U16 => write!(fmt, "u16"),
            TokenData::U32 => write!(fmt, "u32"),
            TokenData::U64 => write!(fmt, "u64"),
            TokenData::S8 => write!(fmt, "s8"),
            TokenData::S16 => write!(fmt, "s16"),
            TokenData::S32 => write!(fmt, "s32"),
            TokenData::S64 => write!(fmt, "s64"),
            TokenData::F32 => write!(fmt, "f32"),
            TokenData::F64 => write!(fmt, "f64"),
            TokenData::STRUCT => write!(fmt, "struct"),
            TokenData::UNION => write!(fmt, "union"),
            TokenData::ENUM => write!(fmt, "enum"),
            TokenData::FUN => write!(fmt, "fun"),
            TokenData::VAR => write!(fmt, "var"),
            TokenData::CONST => write!(fmt, "const"),
            TokenData::DEF => write!(fmt, "def"),
            TokenData::IF => write!(fmt, "if"),
            TokenData::THEN => write!(fmt, "then"),
            TokenData::ELSE => write!(fmt, "else"),
            TokenData::WHILE => write!(fmt, "while"),
            TokenData::LOOP => write!(fmt, "loop"),
            TokenData::MATCH => write!(fmt, "match"),
            TokenData::CASE => write!(fmt, "case"),
            TokenData::BREAK => write!(fmt, "break"),
            TokenData::CONTINUE => write!(fmt, "continue"),
            TokenData::GOTO => write!(fmt, "goto"),
            TokenData::LABEL => write!(fmt, "label"),
            TokenData::SIZEOF => write!(fmt, "sizeof"),
            TokenData::AS => write!(fmt, "as"),
            TokenData::TRUE => write!(fmt, "true"),
            TokenData::FALSE => write!(fmt, "false"),
            TokenData::NULL => write!(fmt, "null"),
            TokenData::IMPORT => write!(fmt, "import"),
            TokenData::FROM => write!(fmt, "from"),
            TokenData::EXTERN => write!(fmt, "extern"),
            TokenData::USING => write!(fmt, "using"),
            TokenData::C8 => write!(fmt, "c8"),
            TokenData::C16 => write!(fmt, "c16"),
            TokenData::C32 => write!(fmt, "c32"),
            TokenData::E32 => write!(fmt, "e32"),
            TokenData::ANY => write!(fmt, "any"),
            TokenData::INFER => write!(fmt, "infer"),
            TokenData::TUPLE => write!(fmt, "tuple"),
            TokenData::TYPE => write!(fmt, "type"),
            TokenData::LET => write!(fmt, "let"),
            TokenData::REF => write!(fmt, "ref"),
            TokenData::VOLAT => write!(fmt, "volat"),
            TokenData::INLINE => write!(fmt, "inline"),
            TokenData::ALIGN => write!(fmt, "align"),
            TokenData::FOR => write!(fmt, "for"),
            TokenData::RAISE => write!(fmt, "raise"),
            TokenData::DEFER => write!(fmt, "defer"),
            TokenData::TRY => write!(fmt, "try"),
            TokenData::CATCH => write!(fmt, "catch"),
            TokenData::YIELD => write!(fmt, "yield"),
            TokenData::ASYNC => write!(fmt, "async"),
            TokenData::AWAIT => write!(fmt, "await"),
            TokenData::NEW => write!(fmt, "new"),
            TokenData::DELETE => write!(fmt, "delete"),
            TokenData::UNDERSCORE => write!(fmt, "underscore"),
            TokenData::THIS => write!(fmt, "this"),
            TokenData::STATIC => write!(fmt, "static"),
            TokenData::DYNAMIC => write!(fmt, "dynamic"),
            TokenData::NAMESPACE => write!(fmt, "namespace"),
            TokenData::ASM => write!(fmt, "asm"),
            TokenData::EXPORT => write!(fmt, "export"),
            TokenData::OPERATOR => write!(fmt, "operator"),
            TokenData::IN => write!(fmt, "in"),
            TokenData::TYPEOF => write!(fmt, "typeof"),
            TokenData::AND => write!(fmt, "and"),
            TokenData::OR => write!(fmt, "or"),
            TokenData::NOT => write!(fmt, "not"),
            TokenData::GEN => write!(fmt, "gen"),
            TokenData::LAMBDA => write!(fmt, "lambda"),
            TokenData::CTE => write!(fmt, "cte"),
            TokenData::PLUS => write!(fmt, "+"),
            TokenData::INCREMENT => write!(fmt, "++"),
            TokenData::MINUS => write!(fmt, "-"),
            TokenData::DECREMENT => write!(fmt, "--"),
            TokenData::EQ => write!(fmt, "="),
            TokenData::EQEQ => write!(fmt, "=="),
            TokenData::NOTEQ => write!(fmt, "!="),
            TokenData::GT => write!(fmt, ">"),
            TokenData::SHR => write!(fmt, ">>"),
            TokenData::LT => write!(fmt, "<"),
            TokenData::SHL => write!(fmt, "<<"),
            TokenData::GTEQ => write!(fmt, ">="),
            TokenData::SLARROW => write!(fmt, "<="),
            TokenData::SRARROW => write!(fmt, "=>"),
            TokenData::OPAREN => write!(fmt, "("),
            TokenData::CPAREN => write!(fmt, ")"),
            TokenData::OTUPLE => write!(fmt, "`("),
            TokenData::OBRACE => write!(fmt, "{{"),
            TokenData::CBRACE => write!(fmt, "}}"),
            TokenData::OSTRUCT => write!(fmt, "`{{"),
            TokenData::OBRACK => write!(fmt, "["),
            TokenData::CBRACK => write!(fmt, "]"),
            TokenData::OARRAY => write!(fmt, "`["),
            TokenData::COMMA => write!(fmt, ","),
            TokenData::DOT => write!(fmt, "."),
            TokenData::CONCAT => write!(fmt, ".."),
            TokenData::SPREAD => write!(fmt, "..."),
            TokenData::DOTPAREN => write!(fmt, ".("),
            TokenData::DOTBRACE => write!(fmt, ".{{"),
            TokenData::DOTBRACK => write!(fmt, ".["),
            TokenData::COLON => write!(fmt, ":"),
            TokenData::BIND => write!(fmt, "::"),
            TokenData::LARROW => write!(fmt, "<-"),
            TokenData::RARROW => write!(fmt, "->"),
            TokenData::BANG => write!(fmt, "!"),
            TokenData::LNOT => write!(fmt, "!!"),
            TokenData::AMPERSAND => write!(fmt, "&"),
            TokenData::LAND => write!(fmt, "&&"),
            TokenData::VBAR => write!(fmt, "|"),
            TokenData::LOR => write!(fmt, "||"),
            TokenData::CARET => write!(fmt, "^"),
            TokenData::LXOR => write!(fmt, "^^"),
            TokenData::AT => write!(fmt, "@"),
            TokenData::HASH => write!(fmt, "#"),
            TokenData::PERCENT => write!(fmt, "%"),
            TokenData::BITSET => write!(fmt, "@|"),
            TokenData::BITCLEAR => write!(fmt, "@&"),
            TokenData::BITFLIP => write!(fmt, "@^"),
            TokenData::BITCHECK => write!(fmt, "@?"),
            TokenData::ASTERISK => write!(fmt, "*"),
            TokenData::SQUIGGLY => write!(fmt, "~"),
            TokenData::SEMICOLON => write!(fmt, ";"),
            TokenData::SLASH => write!(fmt, "/"),
            TokenData::QM => write!(fmt, "?"),
            TokenData::PLUSEQ => write!(fmt, "+="),
            TokenData::MINUSEQ => write!(fmt, "-="),
            TokenData::ASTERISKEQ => write!(fmt, "*="),
            TokenData::SLASHEQ => write!(fmt, "/="),
            TokenData::PERCENTEQ => write!(fmt, "%="),
            TokenData::SHLEQ => write!(fmt, "<<="),
            TokenData::SHREQ => write!(fmt, ">>="),
            TokenData::ANDEQ => write!(fmt, "&="),
            TokenData::OREQ => write!(fmt, "|="),
            TokenData::XOREQ => write!(fmt, "^="),
            TokenData::BITSETEQ => write!(fmt, "@|="),
            TokenData::BITCLEAREQ => write!(fmt, "@&="),
            TokenData::BITFLIPEQ => write!(fmt, "@^="),
            TokenData::CONCATEQ => write!(fmt, "..="),
            TokenData::DIAMOND => write!(fmt, "<>"),
            TokenData::WLARROW => write!(fmt, "<~"),
            TokenData::WRARROW => write!(fmt, "~>"),
            TokenData::LPIPE => write!(fmt, "<|"),
            TokenData::RPIPE => write!(fmt, "|>"),
            TokenData::NULLISH => write!(fmt, "??"),
            TokenData::BACKTICK => write!(fmt, "`"),
            TokenData::NOTEOPEN => write!(fmt, "$["),
            TokenData::DEFAULT => write!(fmt, "/* DEFAULT TOKEN */"),
            TokenData::ERROR(s) => write!(fmt, "/* ERROR: {} */", s),
            TokenData::LINEBREAK => write!(fmt, "\n"),
        }
    }
}

pub struct LexInfo {
    pub tokens: Vec<Token>,
}

#[allow(dead_code)]
impl LexInfo {
    pub fn recreate(&self) -> String {
        self.tokens.iter().map(Token::to_string).collect::<Vec<_>>().join(" ")
    }

    pub fn sanitized(&self) -> Vec<Token> {
        self.tokens.iter().filter(|tok| match tok.t {
            TokenData::LINEBREAK | TokenData::COMMENT{..} => false,
            _ => true
        }).cloned().collect()
    }
}

pub struct Lexer {
    l: u32, // Line
    c: u32, // Column
    p: u32, // Position

    data: Peekable<IntoIter<char>>,
}

impl Lexer {

    pub fn from_file(file_name: &str) -> Result<Self, Error> {
        Ok(Self::from_text(&fs::read_to_string(file_name)?))
    }
    
    pub fn from_text(text: &str) -> Self {
        Lexer { l: 0, c: 0, p: 0, data: text.chars().collect::<Vec<_>>().into_iter().peekable() }
    }

    fn peek(&mut self) -> Option<&char> {
        self.data.peek()
    }

    fn next(&mut self) -> Option<char> {
        self.p += 1;
        self.c += 1;
        self.data.next()
    }

    fn newline(&mut self) {
        self.c = 0;
        self.l += 1;
    }

    fn number(&mut self) -> TokenData {
        use TokenData as T;
        let c = self.next().unwrap(); // Number

        if let Some(&b) = self.peek() {
            let mut s: String; 
            let mut err: bool = false;
            let mut float: bool = false;
            let base: u8;
            if c == '0' {
                match b {
                    'x' => {
                        base = 16;
                        s = String::new();
                    }
                    'o' => {
                        base = 8;
                        s = String::new();
                    }
                    'b' => {
                        base = 2;
                        s = String::new();
                    }
                    '0' ..= '9' | '.' => {
                        base = 10;
                        s = String::new();
                        s.push(c);
                        s.push(b);
                        float = b == '.';
                    }
                    '_' | '\'' => {
                        base = 10;
                        s = String::new();
                        s.push(c);
                    }
                    // [0] is 0 but [1] is not a valid continuation char
                    _ => return T::INT(0) 
                }
            } else {
                base = 10;
                match b {
                    '_' | '\'' => {
                        s = String::new();
                        s.push(c);
                    }
                    '0' ..= '9' | '.' => {
                        s = String::new();
                        s.push(c);
                        s.push(b);
                        float = b == '.';
                    }
                    // [0] is 1..9 but [1] is not a valid continuation char
                    _ => return T::INT(c.to_digit(10).unwrap() as u64)
                }
            }
            self.next(); // b

            // We're on digit 2
            while let Some(&b) = self.peek() {
                match b {
                    '0' ..= '9' => {
                        s.push(b);
                    }
                    '.' => {
                        if base != 10 || float {
                            err = true;
                        } else {
                            float = true;
                        }
                        s.push(b);
                    }
                    '_' | '\'' => (),
                    _ => {
                        if err {
                            return T::ERROR("Invalid number literal".to_string());
                        } else if float {
                            return T::FLOAT(s.parse::<f64>().unwrap());
                        } else {
                            return T::INT(u64::from_str_radix(&s, base as u32).unwrap());
                        }
                    }
                }
                self.next();
            }
            if err {
                T::ERROR("Invalid number literal".to_string())
            } else if float {
                T::FLOAT(s.parse::<f64>().unwrap())
            } else {
                T::INT(u64::from_str_radix(&s, base as u32).unwrap())
            }
        } else {
            T::INT(c.to_digit(10).unwrap() as u64)
        }
    }

    fn escape(&mut self) -> Result<char, String> {
        assert_eq!(self.next().unwrap(), '\\'); // \ 
        match self.peek() {
            Some('0') => {
                self.next(); // 0
                Ok('\0')
            }
            Some('n') => {
                self.next(); // n
                Ok('\n')
            }
            Some('t') => {
                self.next(); // t
                Ok('\t')
            }
            Some('r') => {
                self.next(); // r
                Ok('\r')
            }
            Some('\\') => {
                self.next(); // \\
                Ok('\\')
            }
            Some(&a) => {
                self.next(); // ?
                Ok(a)
            }
            None => {
                Err("Invalid escape".to_string())
            }
        }
    }

    fn string(&mut self) -> Result<String, String> {
        // What happens if this goes wrong??
        assert_eq!(self.next().unwrap(), '"'); // "

        if let Some('"') = self.peek() {
            self.next(); // "
            Ok("".to_string())
        } else {
            let mut s = String::new();
            let mut err: bool = false;

            while let Some(&b) = self.peek() {
                match b {
                    // End of string
                    '"' => {
                        self.next(); // "
                        if err {
                            return Err("Invalid string".to_string())
                        } else {
                            return Ok(s);
                        }
                    }
                    '\\' => {
                        s.push(self.escape()?);
                    }
                    '\n' | '\r' => {
                        self.next(); // \n | \r
                        if let ('\r', Some('\n')) = (b, self.peek()) {
                            self.next(); // \n
                        }
                        self.newline();
                        err = true;
                    }
                    _ => s.push(b)
                }
                self.next();
            }

            Err("Unfinished string".to_string())
        } 
    }

    fn char(&mut self) -> Result<u64, String> {
        assert_eq!(self.next().unwrap(), '\''); // '

        let ch: char;
        match self.peek() {
            Some('\\') => {
                ch = self.escape()?;
            }
            Some('\'') => {
                self.next(); // '
                return Err("Invalid char literal".to_string())
            }
            Some(&b) => {
                self.next();
                ch = b;
            }
            None => return Err("Invalid char literal".to_string())
        }

        match self.peek() {
            Some('\'') => {
                self.next(); // '
                Ok(ch as u64) // Only for nn as C
            }
            _ => {
                Err("Invalid char".to_string())
            }
        }
    }

    pub fn tokenize(&mut self) -> LexInfo {
        let mut res = LexInfo { tokens: Vec::new() };

        while let Some(&c) = self.peek() {
            use TokenData as T;
            let mut tok: Token = Token {t: T::DEFAULT, l: self.l, c: self.c, p: self.p};

            tok.t = match c {
                // Numbers
                '0' ..= '9' => {
                    self.number()
                }
                // String
                '"' => {
                    match self.string() {
                        Ok(s) => {
                            T::STR(s)
                        }
                        Err(s) => {
                            T::ERROR(s)
                        }
                    }
                }
                // Chars
                '\'' => {
                    match self.char() {
                        Ok(u) => {
                            T::INT(u)
                        }
                        Err(s) => {
                            T::ERROR(s)
                        }
                    }
                }
                // Whitespace
                ' ' | '\t' => {
                    self.next(); // whitespace
                    while let Some(' ' | '\t') = self.peek() {
                        self.next();
                    }
                    continue;
                }
                // Newline
                '\n' | '\r' => {
                    self.next(); // \n | \r
                    if let ('\r', Some('\n')) = (c, self.peek()) {
                        self.next(); // \n
                    }
                    self.newline();
                    T::LINEBREAK
                }
                // Identifiers and keywords or underscore
                'a' ..= 'z' | 'A' ..= 'Z' | '_' => {
                    let mut s = String::new();
                    while let Some(&c @ ('0' ..= '9' | 'a' ..= 'z' | 'A' ..= 'Z' | '_')) = self.peek() {
                        self.next();
                        s.push(c);
                    }
                    match s.as_str() {
                        // Types
                        "u0" => T::U0,
                        "u1" => T::U1,
                        "u8" => T::U8,
                        "u16" => T::U16,
                        "u32" => T::U32,
                        "u64" => T::U64,
                        "s8" => T::S8,
                        "s16" => T::S16,
                        "s32" => T::S32,
                        "s64" => T::S64,
                        "f32" => T::F32,
                        "f64" => T::F64,
                        "struct" => T::STRUCT,
                        "union" => T::UNION,
                        "enum" => T::ENUM,
                        "fun" => T::FUN,

                        // Reserved types
                        "c8" => T::C8,
                        "c16" => T::C16,
                        "c32" => T::C32,
                        "e32" => T::E32,
                        "any" => T::ANY,
                        "infer" => T::INFER,
                        "tuple" => T::TUPLE,
                        "type" => T::TYPE,

                        // Boxes
                        "var" => T::VAR,
                        "const" => T::CONST,
                        "def" => T::DEF,

                        // Reserved boxes
                        "let" => T::LET,
                        "ref" => T::REF,
                        "volat" => T::VOLAT,
                        "inline" => T::INLINE,
                        "align" => T::ALIGN,

                        // Flow
                        "if" => T::IF,
                        "then" => T::THEN,
                        "else" => T::ELSE,
                        "while" => T::WHILE,
                        "loop" => T::LOOP,
                        "match" => T::MATCH,
                        "case" => T::CASE,
                        "break" => T::BREAK,
                        "continue" => T::CONTINUE,
                        "goto" => T::GOTO,
                        "label" => T::LABEL,

                        // Reserved flow
                        "for" => T::FOR,
                        "raise" => T::RAISE,
                        "defer" => T::DEFER,
                        "try" => T::TRY,
                        "catch" => T::CATCH,
                        "yield" => T::YIELD,
                        
                        // Operator
                        "sizeof" => T::SIZEOF,
                        "as" => T::AS,
                        "typeof" => T::TYPEOF,
                        "and" => T::AND,
                        "or" => T::OR,
                        "not" => T::NOT,

                        // Reserved operators
                        "new" => T::NEW,
                        "delete" => T::DELETE,

                        // Literals
                        "true" => T::TRUE,
                        "false" => T::FALSE,
                        "null" => T::NULL,
                        
                        // Reserved literals
                        "this" => T::THIS,

                        // Other
                        "import" => T::IMPORT,
                        "from" => T::FROM,
                        "extern" => T::EXTERN,
                        "using" => T::USING,

                        // Reserved other
                        "async" => T::ASYNC,
                        "await" => T::AWAIT,
                        "_" => T::UNDERSCORE,
                        "static" => T::STATIC,
                        "dynamic" => T::DYNAMIC,
                        "namespace" => T::NAMESPACE,
                        "asm" => T::ASM,
                        "export" => T::EXPORT,
                        "operator" => T::OPERATOR,
                        "in" => T::IN,
                        "gen" => T::GEN,
                        "lambda" => T::LAMBDA,
                        "cte" => T::CTE,

                        _ => T::IDEN(s)
                    }
                }
                // Notes or custom identifiers
                '$' => {
                    self.next(); // $
                    match self.peek() {
                        Some('"') => {
                            let res = self.string();
                            if let Ok(s) = res {
                                match s.as_str() {
                                    "" => T::ERROR("Empty identifier string".to_string()),
                                    _ => T::IDEN(s)
                                }
                            } else {
                                T::ERROR(res.unwrap())
                            }
                        }
                        Some('[') => {
                            self.next(); // [
                            T::NOTEOPEN
                        }
                        Some('0' ..= '9' | 'a' ..= 'z' | 'A' ..= 'Z' | '_') => {
                            let mut s = String::new();
                            // This might not work
                            while let Some(&c @ ('0' ..= '9' | 'a' ..= 'z' | 'A' ..= 'Z' | '_')) = self.peek() {
                                self.next();
                                s.push(c);
                            }
                            T::NOTE(s)
                        }
                        _ => T::ERROR("Invalid note".to_string())
                    }
                }
                '+' => {
                    self.next(); // +
                    match self.peek() {
                        Some('+') => { 
                            self.next(); // +
                            T::INCREMENT
                        }
                        Some('=') => {
                            self.next(); // =
                            T::PLUSEQ
                        }
                        _ => T::PLUS
                    }
                }
                '-' => {
                    self.next(); // -
                    match self.peek() {
                        Some('-') => {
                            self.next(); // -
                            T::DECREMENT
                        }
                        Some('>') => {
                            self.next(); // >
                            T::RARROW
                        }
                        Some('=') => {
                            self.next(); // =
                            T::MINUSEQ
                        }
                        _ => T::MINUS
                    }
                }
                '=' => {
                    self.next(); // =
                    match self.peek() {
                        Some('=') => {
                            self.next(); // =
                            T::EQEQ
                        }
                        Some('>') => {
                            self.next(); // >
                            T::SRARROW
                        }
                        _ => T::EQ
                    }
                }
                '>' => {
                    self.next(); // >
                    match self.peek() {
                        Some('=') => {
                            self.next(); // =
                            T::GTEQ
                        }
                        Some('>') => {
                            self.next(); // >
                            match self.peek() {
                                Some('=') => {
                                    self.next(); // =
                                    T::SHREQ
                                }
                                _ => T::SHR
                            }
                        }
                        _ => T::GT
                    }
                }
                '<' => {
                    self.next(); // <
                    match self.peek() {
                        Some('=') => {
                            self.next(); // =
                            T::SLARROW
                        }
                        Some('<') => {
                            self.next(); // <
                            match self.peek() {
                                Some('=') => {
                                    self.next(); // =
                                    T::SHLEQ
                                }
                                _ => T::SHL
                            }
                        }
                        Some('-') => {
                            self.next(); // -
                            T::LARROW
                        }
                        Some('~') => {
                            self.next(); // ~
                            T::WLARROW
                        }
                        Some('>') => {
                            self.next(); // >
                            T::DIAMOND
                        }
                        Some('|') => {
                            self.next(); // |
                            T::LPIPE
                        }
                        _ => T::LT
                    }
                }
                '(' => {
                    self.next(); // (
                    T::OPAREN
                }
                ')' => {
                    self.next(); // )
                    T::CPAREN
                }
                '[' => {
                    self.next(); // [
                    T::OBRACK
                }
                ']' => {
                    self.next(); // ]
                    T::CBRACK
                }
                '{' => {
                    self.next(); // {
                    T::OBRACE
                }
                '}' => {
                    self.next(); // }
                    T::CBRACE
                }
                '`' => {
                    self.next(); // `
                    match self.peek() {
                        Some('(') => {
                            self.next(); // (
                            T::OTUPLE
                        }
                        Some('{') => {
                            self.next(); // {
                            T::OSTRUCT
                        }
                        Some('[') => {
                            self.next(); // [
                            T::OARRAY
                        }
                        _ => {
                            T::BACKTICK
                        }
                    }
                }
                '.' => {
                    self.next(); // .
                    match self.peek() {
                        Some('.') => {
                            self.next(); // .
                            match self.peek() {
                                Some('.') => {
                                    self.next(); // .
                                    T::SPREAD
                                }
                                Some('=') => {
                                    self.next(); // =
                                    T::CONCATEQ
                                }
                                _ => T::CONCAT
                            }
                        }
                        Some('(') => {
                            self.next(); // (
                            T::DOTPAREN
                        }
                        Some('{') => {
                            self.next(); // {
                            T::DOTBRACE
                        }
                        Some('[') => {
                            self.next(); // [
                            T::DOTBRACK
                        }
                        _ => T::DOT
                    }
                }
                ':' => {
                    self.next(); // :
                    match self.peek() {
                        Some(':') => {
                            self.next(); // :
                            T::BIND
                        }
                        _ => T::COLON
                    }
                }
                '!' => {
                    self.next(); // !
                    match self.peek() {
                        Some('!') => {
                            self.next(); // !
                            T::LNOT
                        }
                        Some('=') => {
                            self.next(); // 
                            T::NOTEQ
                        }
                        _ => T::BANG
                    }
                }
                '&' => {
                    self.next(); // &
                    match self.peek() {
                        Some('&') => {
                            self.next(); // &
                            T::LAND
                        }
                        Some('=') => {
                            self.next(); // =
                            T::ANDEQ
                        }
                        _ => T::AMPERSAND
                    }
                }
                '|' => {
                    self.next(); // |
                    match self.peek() {
                        Some('|') => {
                            self.next(); // |
                            T::LOR
                        }
                        Some('=') => {
                            self.next(); // =
                            T::OREQ
                        }
                        Some('>') => {
                            self.next(); // >
                            T::RPIPE
                        }
                        _ => T::VBAR
                    }
                }
                '^' => {
                    self.next(); // ^
                    match self.peek() {
                        Some('^') => {
                            self.next(); // ^
                            T::LXOR
                        }
                        Some('=') => {
                            self.next(); // =
                            T::XOREQ
                        }
                        _ => T::CARET
                    }
                }
                '@' => {
                    self.next(); // @
                    match self.peek() {
                        Some('&') => {
                            self.next(); // &
                            match self.peek() {
                                Some('=') => {
                                    self.next(); // =
                                    T::BITCLEAREQ
                                }
                                _ => T::BITCLEAR
                            }
                        }
                        Some('|') => {
                            self.next(); // |
                            match self.peek() {
                                Some('=') => {
                                    self.next(); // =
                                    T::BITSETEQ
                                }
                                _ => T::BITSET
                            }
                        }
                        Some('^') => {
                            self.next(); // ^
                            match self.peek() {
                                Some('=') => {
                                    self.next(); // =
                                    T::BITFLIPEQ
                                }
                                _ => T::BITFLIP
                            }
                        }
                        Some('?') => {
                            self.next(); // ?
                            T::BITCHECK
                        }
                        _ => T::AT
                    }
                }
                '#' => {
                    self.next(); // #
                    T::HASH
                }
                '%' => {
                    self.next(); // %
                    match self.peek() {
                        Some('=') => {
                            self.next(); // =
                            T::PERCENTEQ
                        }
                        _ => T::PERCENT
                    }
                }
                '*' => {
                    self.next(); // *
                    match self.peek() {
                        Some('=') => {
                            self.next(); // =
                            T::ASTERISKEQ
                        }
                        _ => T::ASTERISK
                    }
                }
                '/' => {
                    self.next(); // /
                    match self.peek() {
                        // Comment
                        Some('/') => {
                            self.next(); // /
                            let mut s = String::new();
                            while let Some(&c) = self.peek() {
                                match c {
                                    '\r' => {
                                        self.next(); // \r
                                        if let Some('\n') = self.peek() {
                                            self.next(); // \n
                                            self.newline();
                                            break;
                                        } else {
                                            s.push('\r');
                                        }
                                    }
                                    '\n' => {
                                        self.next(); // \n
                                        self.newline();
                                        break;
                                    }
                                    _ => {
                                        self.next();
                                        s.push(c);
                                    }
                                }
                            }
                            T::COMMENT{text: s, block: false}
                        }
                        // Block comment (Currently no recursion)
                        Some('*') => {
                            self.next(); // *
                            let mut s = String::new();
                            let mut prev = '\0';
                            while let Some(&c) = self.peek() {
                                match c {
                                    '\r' => {
                                        self.next(); // \r
                                        if let Some('\n') = self.peek() {
                                            self.next(); // \n
                                            self.newline();
                                            s.push('\n');
                                        } else {
                                            s.push('\r');
                                        }
                                    }
                                    '\n' => {
                                        self.next(); // \n
                                        self.newline();
                                    }
                                    '/' => {
                                        if prev == '*' {
                                            s.pop();
                                            break;
                                        }
                                        self.next(); // /
                                        s.push(c);
                                    }
                                    _ => {
                                        self.next();
                                        s.push(c);
                                    }
                                }
                                prev = c;
                            }
                            T::COMMENT{text: s, block: true}
                        }
                        Some('=') => {
                            self.next(); // =
                            T::SLASHEQ
                        }
                        _ => T::SLASH
                    }
                }
                '~' => {
                    self.next(); // ~
                    match self.peek() {
                        Some('>') => {
                            self.next(); // >
                            T::WRARROW
                        }
                        _ => T::SQUIGGLY
                    }
                }
                ',' => {
                    self.next(); // ,
                    T::COMMA
                }
                ';' => {
                    self.next(); // ;
                    T::SEMICOLON
                }
                '?' => {
                    self.next(); // ?
                    match self.peek() {
                        Some('?') => {
                            self.next(); // ?
                            T::NULLISH
                        }
                        _ => T::QM
                    }
                }
                _ => {
                    self.next(); // ???
                    T::ERROR("Unknown character found".to_string())
                }
            };
            
            res.tokens.push(tok);
        }

        res
    }
}