<div align="center">

# Security policy

**Private reporting for security issues in kitten**

[README](../README.md) •
[Русский](SECURITY.ru.md) •
[Documentation](../docs/DOCS.md) •
[License](../LICENSE)

<br>

[![Private disclosure](https://img.shields.io/badge/disclosure-private-61966f?style=flat-square)](#reporting-a-vulnerability)
[![Contact](https://img.shields.io/badge/Telegram-reqsd64-26A5E4?style=flat-square&logo=telegram&logoColor=white)](https://t.me/reqsd64)

</div>

## Reporting a vulnerability

Please report suspected vulnerabilities privately through
[Telegram](https://t.me/reqsd64). Do not open a public issue or publish details
before a fix is available.

Include as much of the following information as possible:

- the affected `kitten` version or revision;
- the operating system and build configuration;
- steps or a minimal input that reproduces the problem;
- the expected and actual behavior;
- the potential security impact;
- any suggested mitigation, if known.

Remove credentials, private paths, and other sensitive data from examples.

## Scope

Security reports may include memory-safety errors, unsafe path handling,
symlink or filesystem race issues, terminal escape injection, and other
behavior that could compromise data or the host environment.

Ordinary bugs, usage questions, and feature requests are not security reports.
They can still be sent through the contact above, but should be clearly marked
as non-security-related.

## Disclosure

After receiving a report, the maintainer may request additional information to
confirm and reproduce the issue. Please allow time for investigation and
coordinate public disclosure until a fix or mitigation is ready.
