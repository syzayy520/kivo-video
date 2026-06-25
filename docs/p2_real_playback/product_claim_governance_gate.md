# P2-048 Product Claim Governance Gate Skeleton

## Purpose

Defines product claim governance gate to ensure no false marketing claims. Validates product claims against allowed vocabulary and prevents misleading statements.

## Scope

- Product claim validation rules
- Allowed vocabulary for claims
- False claim detection
- Marketing compliance
- Claim audit trail

## Family Tree (governance_core/)

```
governance_core/
├── product_claim_request.hpp      — Product claim request
├── product_claim_result.hpp       — Product claim validation result
├── claim_validation_rule.hpp      — Validation rules for claims
├── fake_product_claim_service.hpp — Fake service for testing
```

## Contract Types

### ProductClaimRequest

| Field | Type | Description |
|-------|------|-------------|
| claim_id | string | Unique claim identifier |
| claim_text | string | Product claim text |
| claim_category | string | Claim category (performance, quality, feature, compatibility) |
| evidence_refs | vector<string> | References to supporting evidence |
| submission_timestamp | int64_t | When claim was submitted |

### ProductClaimResult

| Field | Type | Description |
|-------|------|-------------|
| claim_id | string | Claim identifier |
| is_valid | bool | Whether claim passes validation |
| validation_errors | vector<string> | List of validation errors |
| confidence_score | float | Confidence in validation (0.0-1.0) |
| requires_review | bool | Whether claim requires manual review |

### ClaimValidationRule

| Field | Type | Description |
|-------|------|-------------|
| rule_id | string | Unique rule identifier |
| rule_name | string | Human-readable rule name |
| rule_type | string | Rule type (vocabulary, evidence, format, category) |
| allowed_values | vector<string> | Allowed values for this rule |
| severity | string | Rule severity (error, warning, info) |

### FakeProductClaimService

12 builder methods:
- build_valid_performance_claim()
- build_invalid_performance_claim()
- build_valid_feature_claim()
- build_invalid_feature_claim()
- build_strict_validation_rule()
- build_relaxed_validation_rule()
- build_vocabulary_rule()
- build_evidence_rule()
- build_approved_claim_result()
- build_rejected_claim_result()
- build_review_required_claim_result()
- build_partial_confidence_claim_result()

## Tests

12 tests covering:
1. Valid performance claim construction
2. Invalid performance claim construction
3. Valid feature claim construction
4. Invalid feature claim construction
5. Strict validation rule construction
6. Relaxed validation rule construction
7. Vocabulary rule construction
8. Evidence rule construction
9. Approved claim result construction
10. Rejected claim result construction
11. Review required claim result construction
12. Partial confidence claim result construction