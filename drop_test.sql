DROP TABLE IF EXISTS lm_paypal_ipns;
DROP TABLE IF EXISTS lm_paypal_subscribers;
DROP TABLE IF EXISTS lm_paypal_subscriptions;
DROP TABLE IF EXISTS lm_paypal_donations;
DROP TABLE IF EXISTS lm_paypal_paid_adverts;
DELETE FROM system WHERE name = 'lm_paypal';
DELETE FROM system WHERE name = 'lm_paypal_subscriptions';
DELETE FROM system WHERE name = 'lm_paypal_donations';
DELETE FROM system WHERE name = 'lm_paypal_paid_adverts';
