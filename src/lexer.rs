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
#[derive(Debug)]
pub struct Token {
    t: TokenData, // Type
    l: u32, // Line
    c: u32, // Column
    p: u32, // Position
}

pub struct LexInfo {
    pub tokens: Vec<Token>,
}

impl LexInfo {
    pub fn recreate(&self) -> String {
        self.tokens.iter().map(|tok| -> String {
            match &tok.t {
                TokenData::IDEN(i) => i.to_string(),
                TokenData::INT(u) => u.to_string(),
                TokenData::FLOAT(f) => f.to_string(),
                TokenData::CHR(c) => c.to_string(),
                TokenData::STR(s) => format!("\"{}\"", s),
                TokenData::NOTE(n) => format!("${}", n),
                TokenData::COMMENT{text: c, block: true} => format!("/*{}*/", c),
                TokenData::COMMENT{text: c, block: false} => format!("//{}\n", c),
                TokenData::U0 => "u0".to_string(),
                TokenData::U1 => "u1".to_string(),
                TokenData::U8 => "u8".to_string(),
                TokenData::U16 => "u16".to_string(),
                TokenData::U32 => "u32".to_string(),
                TokenData::U64 => "u64".to_string(),
                TokenData::S8 => "s8".to_string(),
                TokenData::S16 => "s16".to_string(),
                TokenData::S32 => "s32".to_string(),
                TokenData::S64 => "s64".to_string(),
                TokenData::F32 => "f32".to_string(),
                TokenData::F64 => "f64".to_string(),
                TokenData::STRUCT => "struct".to_string(),
                TokenData::UNION => "union".to_string(),
                TokenData::ENUM => "enum".to_string(),
                TokenData::FUN => "fun".to_string(),
                TokenData::VAR => "var".to_string(),
                TokenData::CONST => "const".to_string(),
                TokenData::DEF => "def".to_string(),
                TokenData::IF => "if".to_string(),
                TokenData::THEN => "then".to_string(),
                TokenData::ELSE => "else".to_string(),
                TokenData::WHILE => "while".to_string(),
                TokenData::LOOP => "loop".to_string(),
                TokenData::MATCH => "match".to_string(),
                TokenData::CASE => "case".to_string(),
                TokenData::BREAK => "break".to_string(),
                TokenData::CONTINUE => "continue".to_string(),
                TokenData::GOTO => "goto".to_string(),
                TokenData::LABEL => "label".to_string(),
                TokenData::SIZEOF => "sizeof".to_string(),
                TokenData::AS => "as".to_string(),
                TokenData::TRUE => "true".to_string(),
                TokenData::FALSE => "false".to_string(),
                TokenData::NULL => "null".to_string(),
                TokenData::IMPORT => "import".to_string(),
                TokenData::FROM => "from".to_string(),
                TokenData::EXTERN => "extern".to_string(),
                TokenData::USING => "using".to_string(),
                TokenData::C8 => "c8".to_string(),
                TokenData::C16 => "c16".to_string(),
                TokenData::C32 => "c32".to_string(),
                TokenData::E32 => "e32".to_string(),
                TokenData::ANY => "any".to_string(),
                TokenData::INFER => "infer".to_string(),
                TokenData::TUPLE => "tuple".to_string(),
                TokenData::TYPE => "type".to_string(),
                TokenData::LET => "let".to_string(),
                TokenData::REF => "ref".to_string(),
                TokenData::VOLAT => "volat".to_string(),
                TokenData::INLINE => "inline".to_string(),
                TokenData::ALIGN => "align".to_string(),
                TokenData::FOR => "for".to_string(),
                TokenData::RAISE => "raise".to_string(),
                TokenData::DEFER => "defer".to_string(),
                TokenData::TRY => "try".to_string(),
                TokenData::CATCH => "catch".to_string(),
                TokenData::YIELD => "yield".to_string(),
                TokenData::ASYNC => "async".to_string(),
                TokenData::AWAIT => "await".to_string(),
                TokenData::NEW => "new".to_string(),
                TokenData::DELETE => "delete".to_string(),
                TokenData::UNDERSCORE => "underscore".to_string(),
                TokenData::THIS => "this".to_string(),
                TokenData::STATIC => "static".to_string(),
                TokenData::DYNAMIC => "dynamic".to_string(),
                TokenData::NAMESPACE => "namespace".to_string(),
                TokenData::ASM => "asm".to_string(),
                TokenData::EXPORT => "export".to_string(),
                TokenData::OPERATOR => "operator".to_string(),
                TokenData::IN => "in".to_string(),
                TokenData::TYPEOF => "typeof".to_string(),
                TokenData::AND => "and".to_string(),
                TokenData::OR => "or".to_string(),
                TokenData::NOT => "not".to_string(),
                TokenData::GEN => "gen".to_string(),
                TokenData::LAMBDA => "lambda".to_string(),
                TokenData::CTE => "cte".to_string(),
                TokenData::PLUS => "+".to_string(),
                TokenData::INCREMENT => "++".to_string(),
                TokenData::MINUS => "-".to_string(),
                TokenData::DECREMENT => "--".to_string(),
                TokenData::EQ => "=".to_string(),
                TokenData::EQEQ => "==".to_string(),
                TokenData::NOTEQ => "!=".to_string(),
                TokenData::GT => ">".to_string(),
                TokenData::SHR => ">>".to_string(),
                TokenData::LT => "<".to_string(),
                TokenData::SHL => "<<".to_string(),
                TokenData::GTEQ => ">=".to_string(),
                TokenData::SLARROW => "<=".to_string(),
                TokenData::SRARROW => "=>".to_string(),
                TokenData::OPAREN => "(".to_string(),
                TokenData::CPAREN => ")".to_string(),
                TokenData::OTUPLE => "`(".to_string(),
                TokenData::OBRACE => "{".to_string(),
                TokenData::CBRACE => "}".to_string(),
                TokenData::OSTRUCT => "`{".to_string(),
                TokenData::OBRACK => "[".to_string(),
                TokenData::CBRACK => "]".to_string(),
                TokenData::OARRAY => "`[".to_string(),
                TokenData::COMMA => ",".to_string(),
                TokenData::DOT => ".".to_string(),
                TokenData::CONCAT => "..".to_string(),
                TokenData::SPREAD => "...".to_string(),
                TokenData::DOTPAREN => ".(".to_string(),
                TokenData::DOTBRACE => ".{".to_string(),
                TokenData::DOTBRACK => ".[".to_string(),
                TokenData::COLON => ":".to_string(),
                TokenData::BIND => "::".to_string(),
                TokenData::LARROW => "<-".to_string(),
                TokenData::RARROW => "->".to_string(),
                TokenData::BANG => "!".to_string(),
                TokenData::LNOT => "!!".to_string(),
                TokenData::AMPERSAND => "&".to_string(),
                TokenData::LAND => "&&".to_string(),
                TokenData::VBAR => "|".to_string(),
                TokenData::LOR => "||".to_string(),
                TokenData::CARET => "^".to_string(),
                TokenData::LXOR => "^^".to_string(),
                TokenData::AT => "@".to_string(),
                TokenData::HASH => "#".to_string(),
                TokenData::PERCENT => "%".to_string(),
                TokenData::BITSET => "@|".to_string(),
                TokenData::BITCLEAR => "@&".to_string(),
                TokenData::BITFLIP => "@^".to_string(),
                TokenData::BITCHECK => "@?".to_string(),
                TokenData::ASTERISK => "*".to_string(),
                TokenData::SQUIGGLY => "~".to_string(),
                TokenData::SEMICOLON => ";".to_string(),
                TokenData::SLASH => "/".to_string(),
                TokenData::QM => "?".to_string(),
                TokenData::PLUSEQ => "+=".to_string(),
                TokenData::MINUSEQ => "-=".to_string(),
                TokenData::ASTERISKEQ => "*=".to_string(),
                TokenData::SLASHEQ => "/=".to_string(),
                TokenData::PERCENTEQ => "%=".to_string(),
                TokenData::SHLEQ => "<<=".to_string(),
                TokenData::SHREQ => ">>=".to_string(),
                TokenData::ANDEQ => "&=".to_string(),
                TokenData::OREQ => "|=".to_string(),
                TokenData::XOREQ => "^=".to_string(),
                TokenData::BITSETEQ => "@|=".to_string(),
                TokenData::BITCLEAREQ => "@&=".to_string(),
                TokenData::BITFLIPEQ => "@^=".to_string(),
                TokenData::CONCATEQ => "..=".to_string(),
                TokenData::DIAMOND => "<>".to_string(),
                TokenData::WLARROW => "<~".to_string(),
                TokenData::WRARROW => "~>".to_string(),
                TokenData::LPIPE => "<|".to_string(),
                TokenData::RPIPE => "|>".to_string(),
                TokenData::NULLISH => "??".to_string(),
                TokenData::BACKTICK => "`".to_string(),
                TokenData::NOTEOPEN => "$[".to_string(),
                TokenData::DEFAULT => "/* DEFAULT TOKEN */".to_string(),
                TokenData::ERROR(s) => format!("/* ERROR: {} */", s),
                TokenData::LINEBREAK => "\n".to_string(),
            }
        }).collect::<Vec<_>>().join(" ")
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