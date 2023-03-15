#include <columns.h>
#include <assert.h>

void clVisitChildren(
    const clHandler *handler,
    const clColumn *column,
    void *context)
{
    switch (column->kind)
    {
        case cl_COLUMN_INT8:
        case cl_COLUMN_INT16:
        case cl_COLUMN_INT32:
        case cl_COLUMN_INT64:
        case cl_COLUMN_INT128:
        case cl_COLUMN_INT256:
        case cl_COLUMN_UINT8:
        case cl_COLUMN_UINT16:
        case cl_COLUMN_UINT32:
        case cl_COLUMN_UINT64:
        case cl_COLUMN_UINT128:
        case cl_COLUMN_UINT256:
        case cl_COLUMN_FLOAT8:
        case cl_COLUMN_FLOAT16:
        case cl_COLUMN_FLOAT32:
        case cl_COLUMN_FLOAT64:
        case cl_COLUMN_FLOAT128:
        case cl_COLUMN_FLOAT256:
        case cl_COLUMN_BOOL:
            return handler->visitNumber(
                handler, 
                column, 
                context);

        case cl_COLUMN_OBJECT:
            return handler->visitObject(
                handler, 
                column, 
                context);

        case cl_COLUMN_UNION:
            return handler->visitUnion(
                handler, 
                column, 
                context);

        case cl_COLUMN_FIXED_ARRAY:
            return handler->visitFixedArray(
                handler, 
                column, 
                context);

        case cl_COLUMN_FLEXIBLE_ARRAY:
            return handler->visitFlexibleArray(
                handler, 
                column, 
                context);

        default:
            assert(false);
    }
}
