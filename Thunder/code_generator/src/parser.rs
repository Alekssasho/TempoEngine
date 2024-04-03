use nom::branch::alt;
use nom::bytes::complete::is_a;
use nom::bytes::complete::tag;
use nom::bytes::complete::take;
use nom::bytes::complete::take_until;
use nom::character::complete::alpha1;
use nom::character::complete::alphanumeric1;
use nom::character::complete::multispace0;
use nom::combinator::recognize;
use nom::error::ParseError;
use nom::multi::many0;
use nom::multi::many0_count;
use nom::multi::separated_list0;
use nom::sequence::delimited;
use nom::sequence::pair;
use nom::sequence::tuple;
use nom::IResult;
use nom::Parser;

use crate::structures::Attribute;
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

fn parse_single_attribute(input: &str) -> IResult<&str, Attribute> {
    if is_a::<&str, &str, nom::error::Error<&str>>("Component")(input).is_ok() {
        Ok((input, Attribute::Component))
    } else {
        Err(nom::Err::Error(nom::error::Error::new(
            "",
            nom::error::ErrorKind::IsA,
        )))
    }
}

fn parse_attributes(input: &str) -> IResult<&str, Vec<Attribute>> {
    let (input, _) = tag("[")(input)?;
    let (_, input) = take_until("]")(input)?;
    separated_list0(tag(","), parse_single_attribute)(input)
}

fn parse_structure(input: &str) -> IResult<&str, Structure> {
    let (input, attributes) = if let Ok((left_input, attributes_input)) = recognize(tuple((
        tag::<&str, &str, nom::error::Error<&str>>("["),
        take_until("]"),
        take(1u32),
    )))(input)
    {
        let (_, attributes) = parse_attributes(attributes_input)?;
        (left_input, attributes)
    } else {
        (input, vec![])
    };
    let (input, _) = ws(tag("struct"))(input)?;
    let (input, name) = identifier(input)?;
    let (input, (_, members, _)) = tuple((ws(tag("{")), many0(parse_member), ws(tag("}"))))(input)?;

    Ok((
        input,
        Structure {
            name: name.to_string(),
            members,
            attributes,
            ..Default::default()
        },
    ))
}

pub fn parse_input(input: &str) -> IResult<&str, Vec<Structure>> {
    many0(parse_structure)(input)
}
