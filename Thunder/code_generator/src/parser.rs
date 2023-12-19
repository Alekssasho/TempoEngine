use nom::branch::alt;
use nom::bytes::complete::tag;
use nom::character::complete::alpha1;
use nom::character::complete::alphanumeric1;
use nom::character::complete::multispace0;
use nom::combinator::recognize;
use nom::error::ParseError;
use nom::multi::many0;
use nom::multi::many0_count;
use nom::sequence::delimited;
use nom::sequence::pair;
use nom::sequence::tuple;
use nom::IResult;
use nom::Parser;

use crate::structures::Member;
use crate::structures::Structure;

extern crate nom;

fn ws<'a, O, E: ParseError<&'a str>, F>(inner: F) -> impl FnMut(&'a str) -> IResult<&'a str, O, E>
where
    F: Parser<&'a str, O, E>,
{
    delimited(multispace0, inner, multispace0)
}

fn identifier(input: &str) -> IResult<&str, &str> {
    recognize(pair(
        alt((alpha1, tag("_"))),
        many0_count(alt((alphanumeric1, tag("_")))),
    ))
    .parse(input)
}

fn parse_member(input: &str) -> IResult<&str, Member> {
    let (input, (name, _, ttype, _)) =
        ws(tuple((ws(identifier), tag(":"), ws(identifier), tag(";"))))(input)?;

    Ok((
        input,
        Member {
            name: name.to_string(),
            ttype: ttype.to_string(),
        },
    ))
}

fn parse_structure(input: &str) -> IResult<&str, Structure> {
    let (input, _) = ws(tag("struct"))(input)?;
    let (input, name) = identifier(input)?;
    let (input, (_, members, _)) = tuple((ws(tag("{")), many0(parse_member), ws(tag("}"))))(input)?;

    Ok((
        input,
        Structure {
            name: name.to_string(),
            members,
        },
    ))
}

pub fn parse_input(input: &str) -> IResult<&str, Vec<Structure>> {
    many0(parse_structure)(input)
}
