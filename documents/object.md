# Capability

## Overview
Capability provides a mechanism to map an integer (cap) to a data area.
`cap_group` contains multiple threads sharing the same mapping.
The first two entries of a `cap_group` always map to a `struct cap_group` which points to itself
and a `struct vmspace` which manages the virtual address mapping of the cap_group.

## Allocation
A typical allocation includes the following three steps:
1. `obj_alloc` allocates an object and returns a data area with user-defined length.
2. Init the object within the data area.
3. `cap_alloc` allocate a cap for the inited object.
Cap should be allocated at last which guarantees corresponding object is observable after initialized.

Here is an example of initializing a pmobject with error handling:
```
	struct pmobject *pmo;
	int cap, ret;

	/* step 1: obj_alloc allocates an object with given type and length */
	pmo = obj_alloc(TYPE_PMO, sizeof(*pmo));
	if (!pmo) {
		ret = -ENOMEM;
		goto out_fail;
	}

	/* step 2: do whatever you like to init the object */
	pmo_init(pmo);

	/*
	 * step 3: cap_alloc allocates an cap on a cap_group
	 * for the object whith given rights
	 */
	cap = cap_alloc(current_cap_group, pmo, 0);
	if (cap < 0) {
		ret = cap;
		goto out_free_obj;
	}

	......
	/* failover example in the following code to free cap */
	if (failed)
		goto out_free_cap;
	......

out_free_cap_pmo:
	/* cap_free frees both the object and cap */
	cap_free(cap_group, stack_pmo_cap);
	/* set pmo to NULL to avoid double free in obj_free */
	pmo = NULL;
out_free_obj_pmo:
	/* obj_free only frees the object */
	obj_free(pmo);
out_failed:
	return ret;

```

## Search and Use
Object can be get and used by `obj_get` and `obj_put` must be invoked after use.
This pair of get/put solves data race of object use and free (see [Destruction](#destruction) for explaination).
Here is an example of getting and using an cap:
```
	struct pmobject *pmo;

	/* get the object by its cap and type */
	pmo = obj_get(current_cap_group, cap, TYPE_PMO);
	if (!pmo) {
		/* if cap does not map to any object or the type is not TYPE_PMO */
		r = -ECAPBILITY;
		goto out_fail;
	}

	/* do whatever you like to the object */
	......

	obj_put(pmo);
```


## Destruction
Every object contains a reference counter,
which counts it is pointed by how many caps and get by `obj_get`.
The counter is substracted when `cap_free` or `obj_put` is invoked.
When the counter reaches 0, the object is freed.
