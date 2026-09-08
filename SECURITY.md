# Security Policy

## Secure Software Development

libCZI is maintained as a ZEISS open source project. To support its security
and safety, everyone contributing to or maintaining this repository must follow
established secure-development practices, including the OpenSSF Best Practices
where applicable.

This includes the following expectations:

- Review code changes for potential security flaws and use available automated
	analysis tools where appropriate.
- Check integrated third-party components and dependencies for known
	vulnerabilities, and mitigate applicable vulnerabilities according to their
	risk.
- Use state-of-the-art security mechanisms. Do not introduce known-broken or
	insecure cryptographic algorithms, key lengths, password requirements, or
	credential-storage approaches.
- Protect the build and release process from unauthorized or malicious
	modification.
- Do not commit private identifiers, tokens, keys, passwords, or other
	authentication or authorization secrets.
- Do not commit personal data. Copyright notices, commit metadata, and GitHub
	account details may inherently contain personal data.
- Remove obsolete code and unused third-party components when they are no
	longer needed.

## Vulnerability Handling

Confirmed vulnerabilities within the scope of this project are assessed and
fixed in proportion to their risk while the affected project version is under
active maintenance. Fixed vulnerabilities are normally documented in the
release notes, unless responsible disclosure or the mitigation strategy
requires a different approach.

## Reporting a Vulnerability

We welcome security- and safety-related feedback. Please use the channel that
matches the potential severity:

- **None or low severity**, such as an outdated dependency or hardening
	suggestion: open a public issue or submit a pull request.
- **Medium, high, or critical severity**, including a suspected or confirmed
	vulnerability: use [GitHub private vulnerability
	reporting](https://docs.github.com/en/code-security/security-advisories/guidance-on-reporting-and-writing-information-about-vulnerabilities/privately-reporting-a-security-vulnerability).
	Do not disclose vulnerability details in a public issue before coordinated
	disclosure is agreed.

If you are unsure whether an issue is security-related, please report it
privately. A security issue may allow access to something that should not be
accessible, or disable something for other people, but these questions are not
exhaustive.

Reports concerning third-party components should also be reported to the
upstream project where appropriate, to help accelerate remediation and limit
impact. Reports that are clearly irrelevant or lack sufficient information to
investigate may be closed without further action.

### What to Include

Please include as much of the following information as possible:

- The source of the affected code or package, such as its full URL.
- The affected version, branch, or commit hash.
- A concise summary understandable by a non-technical reader.
- Steps to reproduce, including relevant environment details.
- Observed and expected behavior.
- Your assessment of impact, along with known mitigations or workarounds.
- Details of a known exploit or harm already caused, if applicable.

Partial reports are welcome. Maintainers will work with reporters to clarify
the details needed to assess and remediate an issue.

## Voluntary CRA Reporting

Users, contributors, and maintainers are encouraged to escalate
vulnerabilities, cyber threats, incidents, and near misses through the defined
reporting process when voluntary reporting to a CSIRT designated as coordinator
or to ENISA may be appropriate. ZEISS will assess, prepare, and document any
such voluntary report.

## Applicability and Ownership

This policy is owned by ZEISS and applies to all employees, external
maintainers, and contributors working on this repository. Questions or
suggestions for improving this policy or its vulnerability management procedure
may be submitted through the repository.
